#include <Arduino.h>
#include <WiFi.h>
#include <MycilaWebSerial.h>

#include "serial_coms/serial_io.h"

#define LED_PIN 2 // Built-in LED on many ESP32 boards

SerialIO serialComs;
AsyncWebServer server(80);
WebSerial webSerial;

const char *ssid = "ESP Bridge";  // Your WiFi SSID
const char *password = "ogopogo1"; // Your WiFi Password

void setup()
{

    WiFi.softAP(ssid, password);

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(200, "text/plain", "Hi! This is WebSerial demo. You can access the WebSerial interface at http://" + WiFi.localIP().toString() + "/webserial"); });

    webSerial.begin(&server);
    webSerial.setBuffer(100);
    server.begin();

    pinMode(LED_PIN, OUTPUT);

    serialComs.begin();

    serialComs.subscribe(1, [](const JsonDocument &doc)
                         {
                             // Handle incoming messages on channel 1
                             webSerial.println("Received on channel 1: " + doc.as<String>());
                             JsonDocument responseDoc;
                             responseDoc["data"] = "pong";
                             responseDoc["timestamp"] = millis();
                             serialComs.publish(1, responseDoc); // Echo back the message
                             //  WebSerial.println("Received on channel 1: " + doc.as<String>());
                         });
}

void loop()
{
    serialComs.updateSubscriber(); // Process incoming data

    static unsigned long lastBlinkTime = 0;
    static bool ledState = LOW;
    const unsigned long blinkInterval = 500; // milliseconds

    unsigned long currentMillis = millis();
    if (currentMillis - lastBlinkTime >= blinkInterval)
    {
        ledState = !ledState;
        digitalWrite(LED_PIN, ledState);
        lastBlinkTime = currentMillis;
        // JsonDocument helloWorldDoc;
        // helloWorldDoc["data"] = "Hello, World!";
        // helloWorldDoc["timestamp"] = currentMillis;

        // serialComs.publish(1, helloWorldDoc); // Echo back the ping message
    }
}