#include "device_bus.h"
#include <stdarg.h>

// Global convenience instance
DeviceBusClient DeviceBus;

// DeviceInfo helper methods implementation
const char *DeviceInfo::getTypeName() const
{
    return deviceTypeToString(device_type);
}

const char *DeviceInfo::getDataTypeName() const
{
    return dataTypeToString(data_type);
}

// DeviceBusClient implementation
DeviceBusClient::DeviceBusClient(const DeviceBusConfig &config)
    : config_(config), initialized_(false), last_error_(DeviceBusStatus::OK), stats_{}, device_cache_(nullptr), cached_device_count_(0), cache_valid_(false), device_discovered_callback_(nullptr), error_callback_(nullptr), data_received_callback_(nullptr)
{
}

bool DeviceBusClient::begin()
{
    return begin(config_);
}

bool DeviceBusClient::begin(const DeviceBusConfig &config)
{
    config_ = config;

    debugPrint("Initializing Device Bus Client...");

    // Initialize I2C
    config_.wire_instance->begin(config_.sda_pin, config_.scl_pin);
    config_.wire_instance->setClock(config_.frequency);

    // Test connection
    if (!ping())
    {
        setError(DeviceBusStatus::I2C_ERROR, "Failed to connect to slave device");
        return false;
    }

    // Initialize device cache
    if (!refreshDeviceCache())
    {
        debugPrint("Warning: Failed to build initial device cache");
    }

    initialized_ = true;
    debugPrint("Device Bus Client initialized successfully");

    return true;
}

void DeviceBusClient::end()
{
    if (device_cache_)
    {
        delete[] device_cache_;
        device_cache_ = nullptr;
    }

    cached_device_count_ = 0;
    cache_valid_ = false;
    initialized_ = false;

    debugPrint("Device Bus Client ended");
}

void DeviceBusClient::setConfig(const DeviceBusConfig &config)
{
    config_ = config;
    if (initialized_)
    {
        debugPrint("Configuration changed - re-initialization recommended");
    }
}

bool DeviceBusClient::isConnected()
{
    return ping();
}

bool DeviceBusClient::ping()
{
    uint8_t rx_len;
    bool result = executeCommand(DeviceBusCommands::NOP, nullptr, 0, rx_buffer_, rx_len);
    return result;
}

const char *DeviceBusClient::getLastErrorMessage() const
{
    return statusCodeToString(last_error_);
}

uint8_t DeviceBusClient::getDeviceCount()
{
    uint8_t rx_len;

    if (!executeCommand(DeviceBusCommands::GET_DEVICE_COUNT, nullptr, 0, rx_buffer_, rx_len))
    {
        return 0;
    }

    if (rx_len >= 1)
    {
        return rx_buffer_[0];
    }

    return 0;
}

bool DeviceBusClient::getDeviceInfo(uint8_t device_id, DeviceInfo &info)
{
    uint8_t rx_len;

    if (!executeCommand(DeviceBusCommands::GET_DEVICE_INFO, &device_id, 1, rx_buffer_, rx_len))
    {
        return false;
    }

    if (rx_len >= sizeof(DeviceInfo))
    {
        memcpy(&info, rx_buffer_, sizeof(DeviceInfo));

        if (device_discovered_callback_)
        {
            device_discovered_callback_(info);
        }

        return true;
    }

    setError(DeviceBusStatus::ERROR, "Invalid device info response");
    return false;
}

bool DeviceBusClient::getDeviceList(DeviceInfo *devices, uint8_t max_devices, uint8_t &actual_count)
{
    uint8_t rx_len;

    if (!executeCommand(DeviceBusCommands::GET_DEVICE_LIST, nullptr, 0, rx_buffer_, rx_len))
    {
        return false;
    }

    if (rx_len < 1)
    {
        setError(DeviceBusStatus::ERROR, "Invalid device list response");
        return false;
    }

    actual_count = rx_buffer_[0];
    uint8_t devices_to_copy = min(actual_count, max_devices);

    uint8_t offset = 1;
    for (uint8_t i = 0; i < devices_to_copy && (offset + sizeof(DeviceInfo)) <= rx_len; i++)
    {
        memcpy(&devices[i], &rx_buffer_[offset], sizeof(DeviceInfo));
        offset += sizeof(DeviceInfo);

        if (device_discovered_callback_)
        {
            device_discovered_callback_(devices[i]);
        }
    }

    return true;
}

bool DeviceBusClient::findDeviceByName(const char *name, DeviceInfo &info)
{
    if (!cache_valid_ && !refreshDeviceCache())
    {
        return false;
    }

    for (uint8_t i = 0; i < cached_device_count_; i++)
    {
        if (strcmp(device_cache_[i].name, name) == 0)
        {
            info = device_cache_[i];
            return true;
        }
    }

    setError(DeviceBusStatus::DEVICE_NOT_FOUND, "Device not found by name");
    return false;
}

bool DeviceBusClient::findDevicesByType(uint8_t device_type, DeviceInfo *devices, uint8_t max_devices, uint8_t &found_count)
{
    if (!cache_valid_ && !refreshDeviceCache())
    {
        return false;
    }

    found_count = 0;

    for (uint8_t i = 0; i < cached_device_count_ && found_count < max_devices; i++)
    {
        if (device_cache_[i].device_type == device_type)
        {
            devices[found_count] = device_cache_[i];
            found_count++;
        }
    }

    return true;
}

bool DeviceBusClient::readDevice(uint8_t device_id, void *data, uint8_t &data_len)
{
    uint8_t rx_len;

    if (!executeCommand(DeviceBusCommands::READ_DEVICE, &device_id, 1, rx_buffer_, rx_len))
    {
        return false;
    }

    if (rx_len > 0 && data)
    {
        uint8_t copy_len = min(data_len, rx_len);
        memcpy(data, rx_buffer_, copy_len);
        data_len = copy_len;

        if (data_received_callback_)
        {
            data_received_callback_(device_id, data, copy_len);
        }

        return true;
    }

    data_len = 0;
    return false;
}

bool DeviceBusClient::writeDevice(uint8_t device_id, const void *data, uint8_t data_len)
{
    if (data_len >= (I2C_DEVICE_BUS_BUFFER_SIZE - 1))
    {
        setError(DeviceBusStatus::ERROR, "Data too large");
        return false;
    }

    tx_buffer_[0] = device_id;
    if (data && data_len > 0)
    {
        memcpy(&tx_buffer_[1], data, data_len);
    }

    uint8_t rx_len;
    return executeCommand(DeviceBusCommands::WRITE_DEVICE, tx_buffer_, data_len + 1, rx_buffer_, rx_len);
}

bool DeviceBusClient::readDeviceByName(const char *name, void *data, uint8_t &data_len)
{
    uint8_t name_len = strlen(name);
    if (name_len >= MAX_DEVICE_NAME_LEN)
    {
        setError(DeviceBusStatus::ERROR, "Name too long");
        return false;
    }

    uint8_t rx_len;
    if (!executeCommand(DeviceBusCommands::READ_DEVICE_BY_NAME, (const uint8_t *)name, name_len, rx_buffer_, rx_len))
    {
        return false;
    }

    if (rx_len > 0 && data)
    {
        uint8_t copy_len = min(data_len, rx_len);
        memcpy(data, rx_buffer_, copy_len);
        data_len = copy_len;

        if (data_received_callback_)
        {
            // Find device ID for callback
            DeviceInfo info;
            if (findDeviceByName(name, info))
            {
                data_received_callback_(info.device_id, data, copy_len);
            }
        }

        return true;
    }

    data_len = 0;
    return false;
}

bool DeviceBusClient::writeDeviceByName(const char *name, const void *data, uint8_t data_len)
{
    uint8_t name_len = strlen(name);
    if (name_len >= MAX_DEVICE_NAME_LEN || (name_len + data_len + 1) >= I2C_DEVICE_BUS_BUFFER_SIZE)
    {
        setError(DeviceBusStatus::ERROR, "Data too large");
        return false;
    }

    tx_buffer_[0] = name_len;
    memcpy(&tx_buffer_[1], name, name_len);
    if (data && data_len > 0)
    {
        memcpy(&tx_buffer_[1 + name_len], data, data_len);
    }

    uint8_t rx_len;
    return executeCommand(DeviceBusCommands::WRITE_DEVICE_BY_NAME, tx_buffer_, name_len + data_len + 1, rx_buffer_, rx_len);
}

bool DeviceBusClient::getSystemStatus(SystemStatus &status)
{
    uint8_t rx_len;

    if (!executeCommand(DeviceBusCommands::GET_SYSTEM_STATUS, nullptr, 0, rx_buffer_, rx_len))
    {
        return false;
    }

    if (rx_len >= sizeof(SystemStatus))
    {
        memcpy(&status, rx_buffer_, sizeof(SystemStatus));
        return true;
    }

    setError(DeviceBusStatus::ERROR, "Invalid system status response");
    return false;
}

// Type-specific helper implementations
bool DeviceBusClient::readBME280(uint8_t device_id, BME280Data &data)
{
    return readDeviceTyped(device_id, data);
}

bool DeviceBusClient::readBME280ByName(const char *name, BME280Data &data)
{
    return readDeviceTypedByName(name, data);
}

bool DeviceBusClient::readBME280(uint8_t device_id, float &temperature, float &humidity, float &pressure, bool &valid)
{
    BME280Data data;
    if (readBME280(device_id, data))
    {
        temperature = data.temperature;
        humidity = data.humidity;
        pressure = data.pressure;
        valid = data.valid;
        return true;
    }
    return false;
}

bool DeviceBusClient::readBME280ByName(const char *name, float &temperature, float &humidity, float &pressure, bool &valid)
{
    BME280Data data;
    if (readBME280ByName(name, data))
    {
        temperature = data.temperature;
        humidity = data.humidity;
        pressure = data.pressure;
        valid = data.valid;
        return true;
    }
    return false;
}

bool DeviceBusClient::readGPIO(uint8_t device_id, bool &state)
{
    GPIOData data;
    if (readDeviceTyped(device_id, data))
    {
        state = (data.state != 0);
        return true;
    }
    return false;
}

bool DeviceBusClient::readGPIOByName(const char *name, bool &state)
{
    GPIOData data;
    if (readDeviceTypedByName(name, data))
    {
        state = (data.state != 0);
        return true;
    }
    return false;
}

bool DeviceBusClient::writeGPIO(uint8_t device_id, bool state)
{
    GPIOData data = {0};
    data.state = state ? 1 : 0;
    return writeDeviceTyped(device_id, data);
}

bool DeviceBusClient::writeGPIOByName(const char *name, bool state)
{
    GPIOData data = {0};
    data.state = state ? 1 : 0;
    return writeDeviceTypedByName(name, data);
}

bool DeviceBusClient::readADC(uint8_t device_id, uint16_t &raw_value, float &voltage)
{
    ADCData data;
    if (readDeviceTyped(device_id, data))
    {
        raw_value = data.raw_value;
        voltage = data.voltage;
        return true;
    }
    return false;
}

bool DeviceBusClient::readADCByName(const char *name, uint16_t &raw_value, float &voltage)
{
    ADCData data;
    if (readDeviceTypedByName(name, data))
    {
        raw_value = data.raw_value;
        voltage = data.voltage;
        return true;
    }
    return false;
}

bool DeviceBusClient::writePWM(uint8_t device_id, uint16_t duty_cycle, uint16_t frequency)
{
    PWMData data;
    data.duty_cycle = duty_cycle;
    data.frequency = frequency;
    return writeDeviceTyped(device_id, data);
}

bool DeviceBusClient::writePWMByName(const char *name, uint16_t duty_cycle, uint16_t frequency)
{
    PWMData data;
    data.duty_cycle = duty_cycle;
    data.frequency = frequency;
    return writeDeviceTypedByName(name, data);
}

bool DeviceBusClient::writePWMDuty(uint8_t device_id, uint16_t duty_cycle)
{
    // Read current settings first, then update duty cycle
    PWMData current_data;
    if (readDeviceTyped(device_id, current_data))
    {
        current_data.duty_cycle = duty_cycle;
        return writeDeviceTyped(device_id, current_data);
    }

    // If read fails, write with default frequency
    return writePWM(device_id, duty_cycle, 1000);
}

bool DeviceBusClient::writePWMDutyByName(const char *name, uint16_t duty_cycle)
{
    // Read current settings first, then update duty cycle
    PWMData current_data;
    if (readDeviceTypedByName(name, current_data))
    {
        current_data.duty_cycle = duty_cycle;
        return writeDeviceTypedByName(name, current_data);
    }

    // If read fails, write with default frequency
    return writePWMByName(name, duty_cycle, 1000);
}

bool DeviceBusClient::readAllInputs(DataReceivedCallback callback)
{
    if (!cache_valid_ && !refreshDeviceCache())
    {
        return false;
    }

    bool success = true;
    uint8_t buffer[64]; // Temporary buffer for reading

    for (uint8_t i = 0; i < cached_device_count_; i++)
    {
        if (device_cache_[i].isInput())
        {
            uint8_t data_len = sizeof(buffer);
            if (readDevice(device_cache_[i].device_id, buffer, data_len))
            {
                if (callback)
                {
                    callback(device_cache_[i].device_id, buffer, data_len);
                }
                else if (data_received_callback_)
                {
                    data_received_callback_(device_cache_[i].device_id, buffer, data_len);
                }
            }
            else
            {
                success = false;
            }
        }
    }

    return success;
}

bool DeviceBusClient::refreshDeviceCache()
{
    debugPrint("Refreshing device cache...");

    uint8_t device_count = getDeviceCount();
    if (device_count == 0)
    {
        debugPrint("No devices found");
        return false;
    }

    // Allocate cache
    if (device_cache_)
    {
        delete[] device_cache_;
    }

    device_cache_ = new DeviceInfo[device_count];
    if (!device_cache_)
    {
        setError(DeviceBusStatus::ERROR, "Failed to allocate device cache");
        return false;
    }

    // Get device list
    uint8_t actual_count;
    if (getDeviceList(device_cache_, device_count, actual_count))
    {
        cached_device_count_ = actual_count;
        cache_valid_ = true;
        debugPrintf("Device cache refreshed: %d devices", actual_count);
        return true;
    }

    delete[] device_cache_;
    device_cache_ = nullptr;
    cached_device_count_ = 0;
    cache_valid_ = false;

    setError(DeviceBusStatus::ERROR, "Failed to refresh device cache");
    return false;
}

void DeviceBusClient::printDeviceInfo(const DeviceInfo &info, Print &output)
{
    Serial.printf("Device ID: %d\n", info.device_id);
    Serial.printf("Name: %s\n", info.name);
    Serial.printf("Type: %d\n", info.device_type);
    Serial.printf("Direction: %d\n", info.io_direction);
    Serial.printf("Data Type: %d\n", info.data_type);
    Serial.printf("Data Size: %d bytes\n", info.data_size);
    Serial.println();
}

void DeviceBusClient::debugPrint(const char *message)
{
    if (config_.debug_enabled)
    {
        Serial.println(message);
    }
}

void DeviceBusClient::debugPrintf(const char *format, ...)
{
    if (config_.debug_enabled)
    {
        va_list args;
        va_start(args, format);
        Serial.printf(format, args);
        va_end(args);
        Serial.println();
    }
}

void DeviceBusClient::setError(uint8_t error_code, const char *message)
{
    last_error_ = error_code;
    if (error_callback_)
    {
        error_callback_(error_code, message);
    }
    debugPrint(message);
}

bool DeviceBusClient::executeCommand(uint8_t cmd, const uint8_t *data, uint8_t data_len, uint8_t *rx_data, uint8_t &rx_len)
{
    // Prepare command
    tx_buffer_[0] = cmd;
    if (data && data_len > 0)
    {
        memcpy(&tx_buffer_[1], data, data_len);
    }

    // Send command
    config_.wire_instance->beginTransmission(config_.slave_address);
    config_.wire_instance->write(tx_buffer_, data_len + 1);
    if (config_.wire_instance->endTransmission() != 0)
    {
        // debug
        debugPrint("I2C error: Failed to send command");
        setError(DeviceBusStatus::I2C_ERROR, "Failed to send command");
        return false;
    }

    // Request response
    config_.wire_instance->requestFrom(config_.slave_address, (uint8_t)I2C_DEVICE_BUS_BUFFER_SIZE);

    // Read response
    rx_len = 0;
    while (config_.wire_instance->available() && rx_len < I2C_DEVICE_BUS_BUFFER_SIZE)
    {
        rx_data[rx_len++] = config_.wire_instance->read();
    }

    if (rx_len == 0)
    {
        debugPrint("I2C error: No response received");
        setError(DeviceBusStatus::TIMEOUT, "No response received");
        return false;
    }

    return true;
}
