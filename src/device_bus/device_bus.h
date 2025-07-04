#pragma once
#include <configuration.h>

#include <Arduino.h>
#include <Wire.h>
#include <vector>

#define ENVIRONMENTAL_SENSOR_ADDRESS 0x76 // Built-in environmental sensor address
#define GYRO_ADDRESS 0x69                 // Built-in gyroscope address
#define ACCELEROMETER_ADDRESS 0x19        // Built-in accelerometer address

struct SensorDevice
{
    uint8_t address;        // I2C address of the device
    uint8_t digitalOutputs; // Number of digital outputs
    uint8_t digitalInputs;  // Number of digital inputs
    uint8_t analogInputs;   // Number of analog inputs
    uint8_t bme280Sensors;  // Number of BME280 sensors
    uint8_t ledCount;       // Number of LEDs
};

struct BME280Sensor
{
    float temperature; // Temperature in Celsius
    float pressure;    // Pressure in hPa
    float humidity;    // Humidity in percentage
};

struct RGB
{
    uint8_t r; // Red component (0-255)
    uint8_t g; // Green component (0-255)
    uint8_t b; // Blue component (0-255)
};

class DeviceBus
{
public:
    void setup();
    void discover();

    // functions to interact with devices
    void setDigitalOutput(uint8_t address, uint8_t index, bool value = false); // we set default to false so if we forget to set a value, it will default to off
    bool getDigitalInput(uint8_t address, uint8_t index);
    int getAnalogInput(uint8_t address, uint8_t index);
    BME280Sensor getBME280Sensor(uint8_t address, uint8_t index = 0); // Default to first sensor if index is not specified
    void setLED(uint8_t address, RGB color, uint8_t index = 0);       // Set LED color at index for device at address (default to first LED if index is not specified)

private:
    std::vector<uint8_t> potentialAddresses; // Store discovered device addresses
    std::vector<SensorDevice> sensorBoards;  // Store discovered sensor devices

    SensorDevice getSensorDevice(uint8_t address);
};