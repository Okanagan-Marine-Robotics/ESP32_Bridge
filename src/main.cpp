#include <Arduino.h>
#include <WiFi.h>
#include <MycilaWebSerial.h>
#include <Wire.h>

#include "serial_coms/serial_io.h"
#include "tasks/motor_control.h"
#include "tasks/signaling_control.h"

#include "device_bus/device_bus.h"

#include "configuration.h"

// SerialIO serialComs;
#if WIFI_ENABLED
AsyncWebServer server(80);
#endif

#if USE_WEBSERIAL
WebSerial webSerial;
#endif

#if WIFI_ENABLED
const char *ssid = WIFI_SSID;
const char *password = WIFI_PASSWORD;
#endif

SerialIO serialio;
DeviceBusClient deviceBus; // Create a global instance of DeviceBus

// Define a function with the correct signature for esp_log_set_vprintf
#if USE_WEBSERIAL
int webSerialVprintf(const char *fmt, va_list args)
{
    char buffer[256];
    int ret = vsnprintf(buffer, sizeof(buffer), fmt, args);
    webSerial.println(buffer);
    return ret;
}
#endif

void serialTask(void *parameter)
{
    // This task is intentionally left empty to allow the main loop to run
    // It is used to keep the task alive and prevent deletion
    for (;;)
    {
        serialio.updateSubscribers(); // Process incoming serial data
        vTaskDelay(pdMS_TO_TICKS(1)); // Delay to prevent busy-waiting
    }
}

void setup()
{
    serialio.begin(); // Initialize serial communication

#if WIFI_ENABLED
    WiFi.softAP(ssid, password);

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(200, "text/plain", "Hi! you've connected to the ESP32 Bridge. You can access the WebSerial interface at http://" + WiFi.localIP().toString() + "/webserial"); });
#endif
#if USE_WEBSERIAL
    webSerial.begin(&server);
    webSerial.setBuffer(100);
    esp_log_level_set("*", LOG_LEVEL);
    esp_log_set_vprintf(webSerialVprintf);
#else
    esp_log_set_vprintf([](const char *fmt, va_list args) -> int
                        {
            // Do nothing, effectively disabling logging to serial
            return 0; });
#endif
#if WIFI_ENABLED
    server.begin();
#endif

    // set 0x01 to i2c address 0x42
    // Wire.begin(SDA, SCL);  // Initialize I2C with default SDA and SCL pins
    // Wire.setClock(100000); // Set I2C clock speed to 100kHz

    // // send 0x01 to device at address 0x42
    // Wire.beginTransmission(0x42);    // Start transmission to device at address 0x42
    // Wire.write(0x42);                // Write 0x01 to the device
    // if (Wire.endTransmission() != 0) // End transmission and check for errors
    // {
    //     Serial.println("Failed to send data to device at address 0x42");
    // }
    // else
    // {
    //     Serial.println("Data sent successfully to device at address 0x42");
    // }

    // create config for device bus
    // DeviceBusConfig config;
    // config.frequency = 100000;   // Set I2C frequency to 100kHz
    // config.slave_address = 0x42; // Set the slave address for the device bus
    // config.debug_enabled = true; // Enable debug output for device bus
    // if (!deviceBus.begin(config))
    // {
    //     Serial.println("Failed to initialize device bus!");
    //     return;
    // }

    // // Get device count
    // uint8_t device_count = deviceBus.getDeviceCount();
    // Serial.printf("Found %d devices\n\n", device_count);

    // // Get device list
    // DeviceInfo devices[10];
    // uint8_t actual_count;

    // if (deviceBus.getDeviceList(devices, 10, actual_count))
    // {
    //     Serial.println("Device List:");
    //     Serial.println("============");
    //     for (uint8_t i = 0; i < actual_count; i++)
    //     {
    //         deviceBus.printDeviceInfo(devices[i]);
    //     }
    // }

    // while (true)
    // {
    //     vTaskDelay(pdMS_TO_TICKS(1000));
    //     // just stop the code for testing
    // }

    QueueHandle_t *motorTaskQueueHandle = setupMotorControl();         // Initialize motor control
    QueueHandle_t *signalingTaskQueueHandle = setupSignalingControl(); // Initialize signaling control

    serialio.subscribe(1, [motorTaskQueueHandle](const JsonDocument &doc)
                       {
                                    // Handle incoming messages on channel 1
                                    JsonDocument *copy = new JsonDocument;
                                    *copy = doc;
                                    // LOG_WEBSERIALLN("Received on channel 1: " + doc.as<String>());
                                    if (xQueueSend(*motorTaskQueueHandle, &copy, 0) != pdPASS) 
                                    { 
                                        delete copy;  // Clean up if send fails
                                    } });

    serialio.subscribe(254, [signalingTaskQueueHandle](const JsonDocument &doc)
                       {
                           JsonDocument *copy = new JsonDocument;
                           *copy = doc;
                           // LOG_WEBSERIALLN("Received on channel 254: " + doc.as<String>());
                           if (xQueueSend(*signalingTaskQueueHandle, &copy, 0) != pdPASS) 
                           { 
                            delete copy;  // Clean up if send fails
                            } });

    // Create a task to handle serial communication
    BaseType_t taskResult = xTaskCreatePinnedToCore(serialTask, "SerialTask", SERIAL_TASK_STACK_SIZE, NULL, SERIAL_TASK_PRIORITY, NULL, 1);
}

void loop()
{
    // remove this task
    vTaskDelete(NULL); // Delete the current task to free resources
}