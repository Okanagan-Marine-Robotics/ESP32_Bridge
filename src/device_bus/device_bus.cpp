#include "device_bus.h"

void DeviceBus::setup()
{
    Wire.setPins(21, 22); // Set SDA and SCL pins (GPIO 21 and 22 are default for ESP32)
    Wire.begin();
    Wire.setClock(100000); // Set I2C clock speed to 100kHz

    bme280.begin(ENVIRONMENTAL_SENSOR_ADDRESS);                            // Initialize the built-in BME280 sensor
    bmi088 = new Bmi088(Wire, ACCELEROMETER_ADDRESS, GYRO_ADDRESS);        // Initialize the BMI088 sensor with I2C addresses
    bmi088->begin();                                                       // Start the BMI088 sensor
    bmi088->setOdr(Bmi088::ODR_1000HZ);                                    // Set the output data rate to 1000Hz
    bmi088->setRange(Bmi088::ACCEL_RANGE_24G, Bmi088::GYRO_RANGE_2000DPS); // Set accelerometer and gyroscope ranges

    // Scan for devices on the bus
    for (uint8_t address = 1; address < 127; ++address)
    {
        if (address == ENVIRONMENTAL_SENSOR_ADDRESS ||
            address == GYRO_ADDRESS ||
            address == ACCELEROMETER_ADDRESS)
        {
            continue; // Skip known non-sensor board addresses
        }
        Wire.beginTransmission(address);
        if (Wire.endTransmission() == 0)
        {
            LOG_WEBSERIALLN("Found device at address: 0x" + String(address, HEX));
            // Serial.println("Found device at address: 0x" + String(address, HEX));
            potentialAddresses.push_back(address);
        }
    }
}

void DeviceBus::discover()
{
    // Clear previous discoveries
    sensorBoards.clear();

    // Check for known devices
    for (uint8_t address : potentialAddresses)
    {
        // send a no-operation command to the device
        Wire.beginTransmission(address);
        // write a 0x00 byte to the device
        Wire.write(0x00);
        if (Wire.endTransmission() != 0)
        {
            LOG_WEBSERIALLN("Device at address 0x" + String(address, HEX) + " did not respond.");
            continue; // Device did not respond, skip it
        }
        // now read if the device responds with it's address
        Wire.requestFrom(address, 1);
        if (Wire.available() > 0)
        {
            uint8_t response = Wire.read();
            if (response == address)
            {
                LOG_WEBSERIALLN("Device at address 0x" + String(address, HEX) + " is responding.");
                // Get sensor device information
                SensorDevice device = getSensorDevice(address);
                if (device.address != 0)
                {
                    sensorBoards.push_back(device);
                    LOG_WEBSERIALLN("Added device at address 0x" + String(device.address, HEX) + " to sensor bus.");
                    // Serial.println("Added device at address 0x" + String(device.address, HEX) + " to sensor bus.");
                }
            }
            else
            {
                LOG_WEBSERIALLN("Device at address 0x" + String(address, HEX) + " responded with unexpected data: 0x" + String(response, HEX));
            }
        }
        else
        {
            LOG_WEBSERIALLN("No data received from device at address: 0x" + String(address, HEX));
        }
    }
}

DeviceBus::SensorDevice DeviceBus::getSensorDevice(uint8_t address)
{
    SensorDevice device = {address, 0, 0, 0, 0, 0}; // Initialize with default values
    Wire.beginTransmission(address);
    Wire.write(0xD1); // Request device information
    if (Wire.endTransmission() != 0)
    {
        LOG_WEBSERIALLN("Device at address 0x" + String(address, HEX) + " did not respond.");
        return device; // Device did not respond, return empty device
    }
    Wire.requestFrom((int)address, sizeof(SensorDevice) - 1);
    if (Wire.available() == sizeof(SensorDevice) - 1)
    {
        // Read the remaining bytes after the address
        Wire.readBytes(((uint8_t *)&device) + 1, sizeof(SensorDevice) - 1);
        LOG_WEBSERIALLN("Device at address 0x" + String(address, HEX) + " has " + String(device.digitalOutputs) + " digital outputs, " +
                        String(device.digitalInputs) + " digital inputs, " +
                        String(device.analogInputs) + " analog inputs, " +
                        String(device.bme280Sensors) + " BME280 sensors, " +
                        String(device.ledCount) + " LEDs.");

        // if we have 1 leds we set it to blue to indicate that the device is connected
        if (device.ledCount > 0)
        {
            RGB color = {0, 0, 255}; // Blue color
            setLED(address, color);  // Set the first LED to blue
        }
    }
    else
    {
        LOG_WEBSERIALLN("Device at address 0x" + String(address, HEX) + " did not return expected data.");
    }
    return device; // Return the device information
};

std::vector<uint8_t> DeviceBus::getBoardAddresses()
{
    std::vector<uint8_t> addresses;
    for (const auto &device : sensorBoards)
    {
        addresses.push_back(device.address);
    }
    return addresses;
}

void DeviceBus::setDigitalOutput(uint8_t address, uint8_t index, bool value)
{

    auto it = std::find_if(sensorBoards.begin(), sensorBoards.end(),
                           [address](const SensorDevice &dev)
                           { return dev.address == address; });

    if (it == sensorBoards.end())
    {
        LOG_WEBSERIALLN("No sensor board found at address 0x" + String(address, HEX));
        return;
    }

    if (index >= it->digitalOutputs)
    {
        LOG_WEBSERIALLN("Invalid digital output index " + String(index) + " for device at address 0x" + String(address, HEX));
        return;
    }

    Wire.beginTransmission(address);
    Wire.write(0x01);
    Wire.write(index);
    Wire.write(value);
    if (Wire.endTransmission() != 0)
    {
        LOG_WEBSERIALLN("Failed to set LED at address 0x" + String(address, HEX) + " index " + String(index));
        return;
    }
}

bool DeviceBus::getDigitalInput(uint8_t address, uint8_t index)
{
    auto it = std::find_if(sensorBoards.begin(), sensorBoards.end(),
                           [address](const SensorDevice &dev)
                           { return dev.address == address; });

    if (it == sensorBoards.end())
    {
        LOG_WEBSERIALLN("No sensor board found at address 0x" + String(address, HEX));
        return false;
    }

    if (index >= it->digitalOutputs)
    {
        LOG_WEBSERIALLN("Invalid digital input index " + String(index) + " for device at address 0x" + String(address, HEX));
        return false;
    }

    Wire.beginTransmission(address);
    Wire.write(0x02);
    Wire.write(index);
    Wire.endTransmission();
    Wire.requestFrom(address, 1);
    if (Wire.available() > 0)
    {
        return Wire.read();
    }
    return false;
}

int DeviceBus::getAnalogInput(uint8_t address, uint8_t index)
{
    auto it = std::find_if(sensorBoards.begin(), sensorBoards.end(),
                           [address](const SensorDevice &dev)
                           { return dev.address == address; });

    if (it == sensorBoards.end())
    {
        LOG_WEBSERIALLN("No sensor board found at address 0x" + String(address, HEX));
        return -1;
    }

    if (index >= it->analogInputs)
    {
        LOG_WEBSERIALLN("Invalid analog input index " + String(index) + " for device at address 0x" + String(address, HEX));
        return -1;
    }

    Wire.beginTransmission(address);
    Wire.write(0x03);
    Wire.write(index);
    Wire.endTransmission();
    Wire.requestFrom(address, 2);

    if (Wire.available() >= 2)
    {
        uint8_t high = Wire.read();
        uint8_t low = Wire.read();
        return (high << 8) | low;
    }
    return -1;
}

DeviceBus::BME280Sensor DeviceBus::getBME280Sensor(uint8_t address)
{
    if (address == 0)
    {
        // if no address is specified, we return the built-in sensor
        address = ENVIRONMENTAL_SENSOR_ADDRESS;
        LOG_WEBSERIALLN("No address specified, using built-in environmental sensor at address 0x" + String(address, HEX));
        BME280Sensor sensor = {0, 0, 0};
        sensor.humidity = bme280.readHumidity();
        sensor.temperature = bme280.readTemperature();
        sensor.pressure = bme280.readPressure();
        LOG_WEBSERIALLN("Built-in BME280 -> Hum: " + String(sensor.humidity) +
                        ", Temp: " + String(sensor.temperature) +
                        ", Press: " + String(sensor.pressure));
        return sensor;
    }

    auto it = std::find_if(sensorBoards.begin(), sensorBoards.end(),
                           [address](const SensorDevice &dev)
                           { return dev.address == address; });

    if (it == sensorBoards.end())
    {
        LOG_WEBSERIALLN("No sensor board found at address 0x" + String(address, HEX));
        return {0, 0, 0};
    }

    Wire.beginTransmission(address);
    Wire.write(0x04); // Command to read BME280 sensor
    Wire.endTransmission();

    BME280Sensor sensor = {0, 0, 0};
    Wire.requestFrom(address, 12);
    if (Wire.available() >= 12)
    {
        uint8_t buffer[12];
        Wire.readBytes(buffer, 12);

        // Decode 3 floats from the buffer
        memcpy(&sensor.humidity, buffer, 4);
        memcpy(&sensor.temperature, buffer + 4, 4);
        memcpy(&sensor.pressure, buffer + 8, 4);

        LOG_WEBSERIALLN("BME280 at address 0x" + String(address, HEX) +
                        " -> Hum: " + String(sensor.humidity) +
                        ", Temp: " + String(sensor.temperature) +
                        ", Press: " + String(sensor.pressure));
    }
    else
    {
        LOG_WEBSERIALLN("Failed to read BME280 sensor data from address 0x" + String(address, HEX));
    }
    return sensor;
}

void DeviceBus::setLED(uint8_t address, RGB color, uint8_t index)
{

    // check to see if this is possible
    // Find the sensor board for the given address
    auto it = std::find_if(sensorBoards.begin(), sensorBoards.end(),
                           [address](const SensorDevice &dev)
                           { return dev.address == address; });

    if (it == sensorBoards.end())
    {
        LOG_WEBSERIALLN("No sensor board found at address 0x" + String(address, HEX));
        return;
    }

    // Check if the index is within the available LED count
    if (index >= it->ledCount)
    {
        LOG_WEBSERIALLN("Invalid LED index " + String(index) + " for device at address 0x" + String(address, HEX));
        return;
    }

    Wire.beginTransmission(address);
    Wire.write(0x05);    // Command to set LED color
    Wire.write(index);   // LED index
    Wire.write(color.r); // Red component
    Wire.write(color.g); // Green component
    Wire.write(color.b); // Blue component
    if (Wire.endTransmission() != 0)
    {
        LOG_WEBSERIALLN("Failed to set LED at address 0x" + String(address, HEX) + " index " + String(index));
        return;
    }
    LOG_WEBSERIALLN("Set LED at address 0x" + String(address, HEX) + " index " + String(index) +
                    " to color (" + String(color.r) + ", " + String(color.g) + ", " + String(color.b) + ")");
    return;
}

DeviceBus::Bmi088Data DeviceBus::getBmi088Sensor()

{

    Bmi088Data data;

    if (bmi088 == nullptr)
    {
        LOG_WEBSERIALLN("BMI088 sensor not initialized.");
        return {};
    }

    bmi088->readSensor(); // Read the sensor data

    data.accel.x = bmi088->getAccelX_mss();
    data.accel.y = bmi088->getAccelY_mss();
    data.accel.z = bmi088->getAccelZ_mss();
    data.gyro.x = bmi088->getGyroX_rads();
    data.gyro.y = bmi088->getGyroY_rads();
    data.gyro.z = bmi088->getGyroZ_rads();
    data.temperature = bmi088->getTemperature_C();
    data.time = bmi088->getTime_ps();

    LOG_WEBSERIALLN("BMI088 -> Accel: (" + String(data.accel.x) + ", " + String(data.accel.y) + ", " + String(data.accel.z) +
                    "), Gyro: (" + String(data.gyro.x) + ", " + String(data.gyro.y) + ", " + String(data.gyro.z) +
                    "), Temp: " + String(data.temperature) + ", Time: " + String(data.time));

    return data;
}