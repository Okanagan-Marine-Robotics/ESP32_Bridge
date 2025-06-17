#pragma once

#include <Arduino.h>
#include <Wire.h>
#include <functional>

// Configuration constants
#ifndef I2C_DEVICE_BUS_BUFFER_SIZE
#define I2C_DEVICE_BUS_BUFFER_SIZE 128
#endif

#ifndef MAX_DEVICE_NAME_LEN
#define MAX_DEVICE_NAME_LEN 32
#endif

#ifndef I2C_DEVICE_BUS_DEFAULT_TIMEOUT
#define I2C_DEVICE_BUS_DEFAULT_TIMEOUT 100
#endif

// Command definitions
namespace DeviceBusCommands
{
    static const uint8_t NOP = 0x00;
    static const uint8_t GET_DEVICE_COUNT = 0x01;
    static const uint8_t GET_DEVICE_LIST = 0x02;
    static const uint8_t GET_DEVICE_INFO = 0x03;
    static const uint8_t READ_DEVICE = 0x04;
    static const uint8_t WRITE_DEVICE = 0x05;
    static const uint8_t READ_DEVICE_BY_NAME = 0x06;
    static const uint8_t WRITE_DEVICE_BY_NAME = 0x07;
    static const uint8_t GET_SYSTEM_STATUS = 0x08;
}

// Status codes
namespace DeviceBusStatus
{
    static const uint8_t OK = 0x00;
    static const uint8_t ERROR = 0x01;
    static const uint8_t INVALID_CMD = 0x02;
    static const uint8_t INVALID_DEVICE = 0x03;
    static const uint8_t DEVICE_NOT_FOUND = 0x04;
    static const uint8_t TIMEOUT = 0x05;
    static const uint8_t I2C_ERROR = 0x06;
}

// Device types
namespace DeviceTypes
{
    static const uint8_t UNKNOWN = 0x00;
    static const uint8_t BME280 = 0x01;
    static const uint8_t GPIO_INPUT = 0x02;
    static const uint8_t GPIO_OUTPUT = 0x03;
    static const uint8_t ADC = 0x04;
    static const uint8_t PWM = 0x05;
    static const uint8_t SERVO = 0x06;
    static const uint8_t STEPPER = 0x07;
    static const uint8_t CUSTOM = 0xFF;
}

// Data types
namespace DataTypes
{
    static const uint8_t UINT8 = 0x00;
    static const uint8_t UINT16 = 0x01;
    static const uint8_t UINT32 = 0x02;
    static const uint8_t INT8 = 0x03;
    static const uint8_t INT16 = 0x04;
    static const uint8_t INT32 = 0x05;
    static const uint8_t FLOAT = 0x06;
    static const uint8_t BME280_DATA = 0x07;
    static const uint8_t GPIO_STATE = 0x08;
    static const uint8_t CUSTOM = 0xFF;
}

// IO Direction
namespace IODirection
{
    static const uint8_t IO_INPUT = 0x00;
    static const uint8_t IO_OUTPUT = 0x01;
    static const uint8_t IO_BIDIRECTIONAL = 0x02;
}

// Data structures
struct DeviceInfo
{
    uint8_t device_id;
    char name[MAX_DEVICE_NAME_LEN];
    uint8_t device_type;
    uint8_t io_direction;
    uint8_t data_type;
    uint8_t data_size;

    // Helper methods
    bool isInput() const { return io_direction == IODirection::IO_INPUT || io_direction == IODirection::IO_BIDIRECTIONAL; }
    bool isOutput() const { return io_direction == IODirection::IO_OUTPUT || io_direction == IODirection::IO_BIDIRECTIONAL; }
    const char *getTypeName() const;
    const char *getDataTypeName() const;
} __attribute__((packed));

struct SystemStatus
{
    uint32_t uptime_ms;
    uint8_t device_count;
    uint8_t last_error;
    uint32_t command_count;
} __attribute__((packed));

struct BME280Data
{
    float temperature;
    float humidity;
    float pressure;
    uint8_t valid;
} __attribute__((packed));

struct GPIOData
{
    uint8_t state;
    uint8_t reserved[3];
} __attribute__((packed));

struct ADCData
{
    uint16_t raw_value;
    float voltage;
} __attribute__((packed));

struct PWMData
{
    uint16_t duty_cycle; // 0-1000 (0.1% resolution)
    uint16_t frequency;  // Hz
} __attribute__((packed));

// Configuration structure
struct DeviceBusConfig
{
    uint8_t slave_address = 0x50;
    int sda_pin = 21;
    int scl_pin = 22;
    uint32_t frequency = 100000;
    uint16_t timeout_ms = I2C_DEVICE_BUS_DEFAULT_TIMEOUT;
    bool debug_enabled = false;
    TwoWire *wire_instance = &Wire;
};

// Event callback types
using DeviceDiscoveredCallback = std::function<void(const DeviceInfo &)>;
using ErrorCallback = std::function<void(uint8_t error_code, const char *message)>;
using DataReceivedCallback = std::function<void(uint8_t device_id, const void *data, uint8_t length)>;

// Main device bus client class
class DeviceBusClient
{
public:
    // Constructor
    explicit DeviceBusClient(const DeviceBusConfig &config = DeviceBusConfig());

    // Initialization
    bool begin();
    bool begin(const DeviceBusConfig &config);
    void end();

    // Configuration
    void setConfig(const DeviceBusConfig &config);
    DeviceBusConfig getConfig() const { return config_; }
    void setTimeout(uint16_t timeout_ms) { config_.timeout_ms = timeout_ms; }
    void setDebugEnabled(bool enabled) { config_.debug_enabled = enabled; }

    // Connection management
    bool isConnected();
    bool ping();
    uint8_t getLastError() const { return last_error_; }
    const char *getLastErrorMessage() const;

    // Device discovery and information
    uint8_t getDeviceCount();
    bool getDeviceInfo(uint8_t device_id, DeviceInfo &info);
    bool getDeviceList(DeviceInfo *devices, uint8_t max_devices, uint8_t &actual_count);
    bool findDeviceByName(const char *name, DeviceInfo &info);
    bool findDevicesByType(uint8_t device_type, DeviceInfo *devices, uint8_t max_devices, uint8_t &found_count);

    // Generic device access
    bool readDevice(uint8_t device_id, void *data, uint8_t &data_len);
    bool writeDevice(uint8_t device_id, const void *data, uint8_t data_len);
    bool readDeviceByName(const char *name, void *data, uint8_t &data_len);
    bool writeDeviceByName(const char *name, const void *data, uint8_t data_len);

    // System status
    bool getSystemStatus(SystemStatus &status);

    // Type-specific helper methods
    bool readBME280(uint8_t device_id, BME280Data &data);
    bool readBME280ByName(const char *name, BME280Data &data);
    bool readBME280(uint8_t device_id, float &temperature, float &humidity, float &pressure, bool &valid);
    bool readBME280ByName(const char *name, float &temperature, float &humidity, float &pressure, bool &valid);

    bool readGPIO(uint8_t device_id, bool &state);
    bool readGPIOByName(const char *name, bool &state);
    bool writeGPIO(uint8_t device_id, bool state);
    bool writeGPIOByName(const char *name, bool state);

    bool readADC(uint8_t device_id, uint16_t &raw_value, float &voltage);
    bool readADCByName(const char *name, uint16_t &raw_value, float &voltage);

    bool writePWM(uint8_t device_id, uint16_t duty_cycle, uint16_t frequency);
    bool writePWMByName(const char *name, uint16_t duty_cycle, uint16_t frequency);
    bool writePWMDuty(uint8_t device_id, uint16_t duty_cycle);
    bool writePWMDutyByName(const char *name, uint16_t duty_cycle);

    // Bulk operations
    bool readAllInputs(DataReceivedCallback callback = nullptr);
    bool refreshDeviceCache();

    // Event callbacks
    void setDeviceDiscoveredCallback(DeviceDiscoveredCallback callback) { device_discovered_callback_ = callback; }
    void setErrorCallback(ErrorCallback callback) { error_callback_ = callback; }
    void setDataReceivedCallback(DataReceivedCallback callback) { data_received_callback_ = callback; }

    // Utility methods
    void printDeviceInfo(const DeviceInfo &info, Print &output = Serial);
    void printSystemStatus(const SystemStatus &status, Print &output = Serial);
    void printAllDevices(Print &output = Serial);
    void printLastError(Print &output = Serial);

    // Statistics
    struct Statistics
    {
        uint32_t commands_sent;
        uint32_t successful_responses;
        uint32_t failed_responses;
        uint32_t timeouts;
        uint32_t i2c_errors;
        unsigned long last_communication_ms;
    };

    Statistics getStatistics() const { return stats_; }
    void resetStatistics();

private:
    // Configuration
    DeviceBusConfig config_;

    // State
    bool initialized_;
    uint8_t last_error_;
    Statistics stats_;

    // Buffers
    uint8_t tx_buffer_[I2C_DEVICE_BUS_BUFFER_SIZE];
    uint8_t rx_buffer_[I2C_DEVICE_BUS_BUFFER_SIZE];

    // Device cache
    DeviceInfo *device_cache_;
    uint8_t cached_device_count_;
    bool cache_valid_;

    // Callbacks
    DeviceDiscoveredCallback device_discovered_callback_;
    ErrorCallback error_callback_;
    DataReceivedCallback data_received_callback_;

    // Internal methods
    bool sendCommand(uint8_t cmd, const uint8_t *data = nullptr, uint8_t data_len = 0);
    bool receiveResponse(uint8_t &status, uint8_t *data, uint8_t &data_len);
    bool executeCommand(uint8_t cmd, const uint8_t *tx_data, uint8_t tx_len, uint8_t *rx_data, uint8_t &rx_len);

    void setError(uint8_t error_code, const char *message = nullptr);
    void debugPrint(const char *message);
    void debugPrintf(const char *format, ...);
    void updateStatistics(bool success, bool timeout = false);

    void invalidateCache();
    bool buildDeviceCache();

    // Template helpers for type-safe operations
    template <typename T>
    bool readDeviceTyped(uint8_t device_id, T &data);

    template <typename T>
    bool writeDeviceTyped(uint8_t device_id, const T &data);

    template <typename T>
    bool readDeviceTypedByName(const char *name, T &data);

    template <typename T>
    bool writeDeviceTypedByName(const char *name, const T &data);
};

// Utility functions
const char *deviceTypeToString(uint8_t device_type);
const char *dataTypeToString(uint8_t data_type);
const char *ioDirectionToString(uint8_t io_direction);
const char *statusCodeToString(uint8_t status_code);

// Global convenience instance (optional)
extern DeviceBusClient DeviceBus;