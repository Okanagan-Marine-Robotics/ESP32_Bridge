#include "device_bus.h"

void DeviceBus::setup()
{
    Wire.setPins(21, 22); // Set SDA and SCL pins (GPIO 21 and 22 are default for ESP32)
    Wire.begin();
    Wire.setClock(100000); // Set I2C clock speed to 100kHz

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

SensorDevice DeviceBus::getSensorDevice(uint8_t address)
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

        // Serial.println("Device at address 0x" + String(address, HEX) + " has " + String(device.digitalOutputs) + " digital outputs, " +
        //                String(device.digitalInputs) + " digital inputs, " +
        //                String(device.analogInputs) + " analog inputs, " +
        //                String(device.bme280Sensors) + " BME280 sensors, " +
        //                String(device.ledCount) + " LEDs.");

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

void DeviceBus::setLED(uint8_t address, RGB color, uint8_t index)
{
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