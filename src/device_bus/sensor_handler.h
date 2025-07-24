#include <Arduino.h>
#include "configuration.h"

class SensorHandler
{
public:
    SensorHandler() = default;
    void startSensorHandler();
    void setAnalogInputConfig(uint8_t boardAddress, uint8_t inputIndex, uint32_t intervalMs, bool enabled, const String &name = "");
    void setDigitalInputConfig(uint8_t boardAddress, uint8_t inputIndex, uint32_t intervalMs, bool enabled, const String &name = "");
    void setBME280Config(uint8_t boardAddress, uint32_t intervalMs, bool enabled);
    void setBMI088Config(uint32_t intervalMs, bool enabled);

private:
    SemaphoreHandle_t i2cMutex = NULL; // Mutex for I2C operations
    TaskHandle_t bmi088TaskHandle = NULL;
    TaskHandle_t bme280TaskHandle = NULL;
    TaskHandle_t analogInputTaskHandle = NULL;
    TaskHandle_t digitalInputTaskHandle = NULL;

    void bmi088SensorTask(void *parameter);
    static void bmi088SensorTaskWrapper(void *parameter);
    void bme280SensorTask(void *parameter);
    static void bme280SensorTaskWrapper(void *parameter);
    void analogInputTask(void *parameter);
    static void analogInputTaskWrapper(void *parameter);
    void digitalInputTask(void *parameter);
    static void digitalInputTaskWrapper(void *parameter);
};