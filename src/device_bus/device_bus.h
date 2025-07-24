#pragma once
#include <configuration.h>

#include <Arduino.h>
#include <Wire.h>
#include <vector>
#include <BMI088.h>
#include <Adafruit_BME280.h>

#define ENVIRONMENTAL_SENSOR_ADDRESS 0x76 // Built-in environmental sensor address
#define GYRO_ADDRESS 0x69                 // Built-in gyroscope address
#define ACCELEROMETER_ADDRESS 0x19        // Built-in accelerometer address

class DeviceBus
{
public:
    void setup();
    void discover();

    struct SensorDevice
    {
        uint8_t address;        // I2C address of the device
        uint8_t digitalOutputs; // Number of digital outputs
        uint8_t digitalInputs;  // Number of digital inputs
        uint8_t analogInputs;   // Number of analog inputs
        uint8_t bme280Sensors;  // Number of BME280 sensors
        uint8_t ledCount;       // Number of LEDs
    };

    struct RGB
    {
        uint8_t r; // Red component (0-255)
        uint8_t g; // Green component (0-255)
        uint8_t b; // Blue component (0-255)
    };

    struct BME280Sensor
    {
        float humidity;    // Humidity in percentage
        float temperature; // Temperature in Celsius
        float pressure;    // Pressure in hPa
    };

    struct Bmi088AccelData
    {
        float x; // Acceleration in X direction in m/s^2
        float y; // Acceleration in Y direction in m/s^2
        float z; // Acceleration in Z direction in m/s^2
        Bmi088AccelData() : x(0.0f), y(0.0f), z(0.0f) {}
    };
    struct Bmi088GyroData
    {
        float x; // Angular velocity in X direction in rad/s
        float y; // Angular velocity in Y direction in rad/s
        float z; // Angular velocity in Z direction in rad/s
        Bmi088GyroData() : x(0.0f), y(0.0f), z(0.0f) {}
    };
    struct Bmi088Data
    {
        Bmi088AccelData accel; // Accelerometer data
        Bmi088GyroData gyro;   // Gyroscope data
        float temperature;     // Temperature in Celsius
        uint64_t time;         // Timestamp in picoseconds
        Bmi088Data() : accel(), gyro(), temperature(0.0f), time(0) {}
    };

    struct InputConfig
    {
        uint32_t samplingIntervalMs;
        bool enabled;
        uint32_t lastSampleTime;
        String inputName; // Optional name for the input
    };

    struct BoardSensorConfig
    {
        uint8_t address;
        std::vector<InputConfig> analogInputs;
        std::vector<InputConfig> digitalInputs;
        InputConfig bme280Config;
        bool hasBME280;
    };

    // functions to interact with devices
    void setDigitalOutput(uint8_t address, uint8_t index, bool value = false); // we set default to false so if we forget to set a value, it will default to off
    bool getDigitalInput(uint8_t address, uint8_t index);
    int getAnalogInput(uint8_t address, uint8_t index);
    BME280Sensor getBME280Sensor(uint8_t address = 0); // Default to first sensor if index is not specified
    Bmi088Data getBmi088Sensor();                      // Onboard device so no address needed
    Bmi088AccelData getBmi088Accel();                  // Onboard device so no address needed
    Bmi088GyroData getBmi088Gyro();                    // Onboard device so no address needed

    void setLED(uint8_t address, RGB color, uint8_t index = 0); // Set LED color at index for device at address (default to first LED if index is not specified)
    std::vector<uint8_t> getBoardAddresses();
    SensorDevice getSensorDevice(uint8_t address);

private:
    std::vector<uint8_t> potentialAddresses; // Store discovered device addresses
    std::vector<SensorDevice> sensorBoards;  // Store discovered sensor devices

    Adafruit_BME280 bme280;   // BME280 sensor built-in instance
    Bmi088 *bmi088 = nullptr; // BMI088 sensor pointer, to be initialized later
};
