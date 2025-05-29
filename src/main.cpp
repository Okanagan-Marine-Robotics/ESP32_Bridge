#include <Arduino.h>
#include <WiFi.h>
#include <MycilaWebSerial.h>

#include "serial_coms/serial_io.h"
#include "tasks/motor_control.h"

SerialIO serialComs;
AsyncWebServer server(80);
WebSerial webSerial;

const char *ssid = "ESP Bridge";   // Your WiFi SSID
const char *password = "ogopogo1"; // Your WiFi Password

// QueueHandle_t channel1Queue;
// #define QUEUE_SIZE 10
// #define STACK_SIZE 4096
// #define PRIORITY 1

// void channel1Task(void *parameter)
// {
//     JsonDocument *doc;
//     for (;;)
//     {
//         if (xQueueReceive(channel1Queue, &doc, portMAX_DELAY) == pdPASS)
//         {
//             webSerial.println("Task Handling Channel 1: " + doc->as<String>());

//             JsonDocument responseDoc;
//             responseDoc["data"] = "pong";
//             responseDoc["timestamp"] = millis();
//             // sleep for 1000ms
//             vTaskDelay(pdMS_TO_TICKS(1000));
//             serialComs.publish(1, responseDoc);

//             delete doc;
//         }
//     }
// }

// Define a function with the correct signature for esp_log_set_vprintf
int webSerialVprintf(const char *fmt, va_list args)
{
    char buffer[256];
    int ret = vsnprintf(buffer, sizeof(buffer), fmt, args);
    webSerial.println(buffer);
    return ret;
}

void setup()
{

    WiFi.softAP(ssid, password);

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(200, "text/plain", "Hi! you've connected to the ESP32 Bridge. You can access the WebSerial interface at http://" + WiFi.localIP().toString() + "/webserial"); });

    webSerial.begin(&server);
    webSerial.setBuffer(100);
    server.begin();
    esp_log_set_vprintf(webSerialVprintf);
    // escDriver.setThrottle(100.0f); // Initialize ESC with 0% throttle
    // pinMode(32, OUTPUT); // Set the built-in LED pin as output
    // digitalWrite(32, HIGH);

    serialComs.begin();
    // channel1Queue = xQueueCreate(QUEUE_SIZE, sizeof(JsonDocument *));
    // xTaskCreatePinnedToCore(channel1Task, "Channel1Task", STACK_SIZE, NULL, PRIORITY, NULL, 1);

    QueueHandle_t *motorTaskQueueHandle = setupMotorControl(); // Initialize motor control

    serialComs.subscribe(1, [motorTaskQueueHandle](const JsonDocument &doc)
                         {
                             // Handle incoming messages on channel 1
                             JsonDocument *copy = new JsonDocument;
                             *copy = doc;
                             xQueueSend(*motorTaskQueueHandle, &copy, 0);
                             //  WebSerial.println("Received on channel 1: " + doc.as<String>());
                         });
    serialComs.subscribe(3, [](const JsonDocument &doc)
                         { webSerial.println("Received on channel 3: " + doc.as<String>()); });
}

void loop()
{
    serialComs.updateSubscriber(); // Process incoming data
}