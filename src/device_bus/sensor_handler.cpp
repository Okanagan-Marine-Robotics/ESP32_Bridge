#include "sensor_handler.h"
#include "device_bus/device_bus.h"
#include "serial_coms/serial_io.h"

DeviceBus deviceBus; // Create a global instance of DeviceBus
extern SerialIO serialio;

void SensorHandler::startSensorHandler()
{
    // Initialize the I2C mutex
    i2cMutex = xSemaphoreCreateMutex();
    if (i2cMutex == NULL)
    {
        LOG_WEBSERIALLN("Failed to create I2C mutex");
        return;
    }

    // Initialize the sensor system with default configurations
    deviceBus.setup();    // Initialize device bus communication
    deviceBus.discover(); // Discover devices on the bus

    // Set up tasks for each sensor type
    xTaskCreatePinnedToCore(
        bmi088SensorTaskWrapper,
        "BMI088 Sensor Task",
        2048,
        this,
        1,
        nullptr,
        1);

    xTaskCreatePinnedToCore(
        bme280SensorTaskWrapper,
        "BME280 Sensor Task",
        2048,
        this,
        1,
        nullptr,
        1);

    xTaskCreatePinnedToCore(
        analogInputTaskWrapper,
        "Analog Input Task",
        2048,
        this,
        1,
        nullptr,
        1);

    xTaskCreatePinnedToCore(
        digitalInputTaskWrapper,
        "Digital Input Task",
        2048,
        this,
        1,
        nullptr,
        1);
}

void SensorHandler::bmi088SensorTaskWrapper(void *parameter)
{
    SensorHandler *instance = static_cast<SensorHandler *>(parameter);
    instance->bmi088SensorTask(parameter);
}

void SensorHandler::bme280SensorTaskWrapper(void *parameter)
{
    SensorHandler *instance = static_cast<SensorHandler *>(parameter);
    instance->bme280SensorTask(parameter);
}

void SensorHandler::analogInputTaskWrapper(void *parameter)
{
    SensorHandler *instance = static_cast<SensorHandler *>(parameter);
    instance->analogInputTask(parameter);
}

void SensorHandler::digitalInputTaskWrapper(void *parameter)
{
    SensorHandler *instance = static_cast<SensorHandler *>(parameter);
    instance->digitalInputTask(parameter);
}

void SensorHandler::bmi088SensorTask(void *parameter)
{
    for (;;)
    {
        if (xSemaphoreTake(i2cMutex, portMAX_DELAY) == pdTRUE)
        {
            DeviceBus::Bmi088Data data = deviceBus.getBmi088Sensor();
            xSemaphoreGive(i2cMutex);

            JsonDocument doc;
            doc["x"] = data.accel.x;
            doc["y"] = data.accel.y;
            doc["z"] = data.accel.z;
            serialio.publish(3, doc);
            doc["x"] = data.gyro.x;
            doc["y"] = data.gyro.y;
            doc["z"] = data.gyro.z;
            serialio.publish(4, doc);
            doc["t"] = data.temperature;
            doc["ti"] = data.time;
            serialio.publish(5, doc);
        }
        vTaskDelay(pdMS_TO_TICKS(100)); // Adjust the delay as needed
    }
}

void SensorHandler::bme280SensorTask(void *parameter)
{
    for (;;)
    {
        if (xSemaphoreTake(i2cMutex, portMAX_DELAY) == pdTRUE)
        {
            // Include address 0 in the list of addresses
            std::vector<uint8_t> addresses = deviceBus.getBoardAddresses();
            addresses.insert(addresses.begin(), 0);

            for (auto &address : addresses)
            {
                DeviceBus::BME280Sensor result = deviceBus.getBME280Sensor(address);
                xSemaphoreGive(i2cMutex);

                JsonDocument doc;
                doc["a"] = address; // Address of the sensor board
                doc["t"] = result.temperature;
                doc["h"] = result.humidity;
                doc["p"] = result.pressure;

                serialio.publish(2, doc);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(100)); // Adjust the delay as needed
    }
}

void SensorHandler::analogInputTask(void *parameter)
{
    for (;;)
    {
        if (xSemaphoreTake(i2cMutex, portMAX_DELAY) == pdTRUE)
        {
            for (auto &address : deviceBus.getBoardAddresses())
            {
                for (uint8_t i = 0; i < deviceBus.getSensorDevice(address).analogInputs; ++i)
                {
                    int value = deviceBus.getAnalogInput(address, i);
                    JsonDocument doc;
                    doc["a"] = address; // Address of the sensor board
                    doc["i"] = i;       // Index of the analog input
                    doc["v"] = value;   // Value read from the analog input

                    serialio.publish(6, doc);
                }
            }
            xSemaphoreGive(i2cMutex);
        }
        vTaskDelay(pdMS_TO_TICKS(100)); // Adjust the delay as needed
    }
}

void SensorHandler::digitalInputTask(void *parameter)
{
    for (;;)
    {
        if (xSemaphoreTake(i2cMutex, portMAX_DELAY) == pdTRUE)
        {
            for (auto &address : deviceBus.getBoardAddresses())
            {
                for (uint8_t i = 0; i < deviceBus.getSensorDevice(address).digitalInputs; ++i)
                {
                    bool value = deviceBus.getDigitalInput(address, i);
                    JsonDocument doc;
                    doc["a"] = address; // Address of the sensor board
                    doc["i"] = i;       // Index of the digital input
                    doc["v"] = value;   // Value read from the digital input

                    serialio.publish(7, doc);
                }
            }
            xSemaphoreGive(i2cMutex);
        }
        vTaskDelay(pdMS_TO_TICKS(100)); // Adjust the delay as needed
    }
}
