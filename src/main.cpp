#include <Arduino.h>
#include <WiFi.h>
#include <MycilaWebSerial.h>

// #include "esp_task_wdt.h"

#include "serial_coms/serial_io.h"
#include "tasks/motor_control.h"
#include "tasks/signaling_control.h"

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
    serialio.begin(); // Initialize serial communication
    LOG_WEBSERIALLN("Serial task started");

    for (;;)
    {
        serialio.updateSubscriber();  // Process incoming messages
        vTaskDelay(pdMS_TO_TICKS(1)); // Yield CPU to other tasks
    }
}

void setup()
{
    // while (!ESP32_SERIAL)
    // {
    //     vTaskDelay(100 / portTICK_PERIOD_MS); // Wait for serial port to connect. Needed for native USB
    // }

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

    QueueHandle_t *motorTaskQueueHandle = setupMotorControl();         // Initialize motor control
    QueueHandle_t *signalingTaskQueueHandle = setupSignalingControl(); // Initialize signaling control

    serialio.subscribe(1, [motorTaskQueueHandle](const JsonDocument &doc)
                       {
                                    // Handle incoming messages on channel 1

                                    JsonDocument *copy = new JsonDocument;
                                    *copy = doc;
                                    // LOG_WEBSERIALLN("Received on channel 1: " + doc.as<String>());
                                    xQueueSend(*motorTaskQueueHandle, &copy, 0); });

    serialio.subscribe(254, [signalingTaskQueueHandle](const JsonDocument &doc)
                       {
                                    JsonDocument *copy = new JsonDocument;
                                    *copy = doc;
                                    // LOG_WEBSERIALLN("Received on channel 254: " + doc.as<String>());
                                    xQueueSend(*signalingTaskQueueHandle, &copy, 0); });

    xTaskCreatePinnedToCore(
        serialTask,             // Task function
        "Serial Task",          // Task name
        SERIAL_TASK_STACK_SIZE, // Stack size
        NULL,                   // Parameters
        10,                     // Priority (higher than default = more responsive)
        NULL,                   // Task handle
        0);                     // Core 1 (or use 0 if core 1 is busy)
}

void loop()
{
    // remove this task
    vTaskDelete(NULL); // Delete the task to free resources
}