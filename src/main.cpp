#include <Arduino.h>
#include <WiFi.h>
#include <MycilaWebSerial.h>
#include <Wire.h>

#include "serial_coms/serial_io.h"
#include "tasks/motor_control.h"
#include "tasks/signaling_control.h"

#include "device_bus/sensor_handler.h"

#include "configuration.h"
#include "tasks/led_control.h"

// SerialIO serialComs;
extern SerialIO serialio; // Declare the global SerialIO instance
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

LedControl ledControl;       // Create an instance of LedControl
SensorHandler sensorHandler; // Create an instance of SensorHandler

void setup()
{

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

#if USE_WEBSERIAL
    delay(STARTUP_DELAY * 1000); // Wait for a short period to allow devices to connect to the WiFi network for logging
#endif

    LOG_WEBSERIALLN("ESP32 Bridge starting up...");
    ledControl.setup(); // Initialize LED control
    serialio.begin();   // Initialize serial communication

    QueueHandle_t *motorTaskQueueHandle = setupMotorControl();         // Initialize motor control
    QueueHandle_t *signalingTaskQueueHandle = setupSignalingControl(); // Initialize signaling control
    sensorHandler.startSensorHandler();                                // Start the sensor handler

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