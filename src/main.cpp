#include <Arduino.h>
#include <WiFi.h>
#include <MycilaWebSerial.h>

#include "serial_coms/serial_io.h"
#include "tasks/motor_control.h"
#include "tasks/signaling_control.h"

#include "configuration.h"

extern "C" void app_main()
{
    initArduino();
    setup();
    while (true)
    {
        loop();
        vTaskDelay(pdMS_TO_TICKS(1)); // Yield to other tasks
    }
}

SerialIO serialComs;
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
    serialComs.begin();

    QueueHandle_t *motorTaskQueueHandle = setupMotorControl();         // Initialize motor control
    QueueHandle_t *signalingTaskQueueHandle = setupSignalingControl(); // Initialize signaling control

    serialComs.subscribe(1, [motorTaskQueueHandle](const JsonDocument &doc)
                         {
                            // Handle incoming messages on channel 1

                            JsonDocument *copy = new JsonDocument;
                            *copy = doc;
                            LOG_WEBSERIALLN("Received on channel 1: " + doc.as<String>());
                            xQueueSend(*motorTaskQueueHandle, &copy, 0); });

    serialComs.subscribe(254, [signalingTaskQueueHandle](const JsonDocument &doc)
                         {
                            JsonDocument *copy = new JsonDocument;
                            *copy = doc;
                            LOG_WEBSERIALLN("Received on channel 254: " + doc.as<String>());
                            xQueueSend(*signalingTaskQueueHandle, &copy, 0); });
}

void loop()
{
    serialComs.updateSubscriber(); // Process incoming data
}