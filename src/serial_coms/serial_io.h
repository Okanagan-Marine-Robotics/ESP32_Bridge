// serial_io.h

#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>
#include <unordered_map>
#include <vector>

#define LED_PIN 2 // Built-in LED on many ESP32 boards

// Callback function type for subscription
using SubscriptionCallback = void (*)(JsonDocument &doc);

class SerialIO
{
public:
    void begin();
    // write arduino json document to serial
    void publish(int channel, const JsonDocument &doc);
    void subscribe(int channel, JsonDocument &doc, SubscriptionCallback callback);
    void pollSerialToQueue();

    using Callback = std::function<void(const JsonDocument &doc)>;
    void subscribe(uint8_t channel, Callback cb);
    void updateSubscriber(); // call frequently to process incoming data
    bool available();

private:
    size_t write(const uint8_t *buffer, size_t size);
    size_t readBytes(uint8_t *buffer, size_t length);
    void flush();

    std::unordered_map<uint8_t, Callback> _callbacks;
    std::vector<uint8_t> _buffer;
    void _processPacket(const std::vector<uint8_t> &packet);

    QueueHandle_t _rxQueue;
    void onUartRx();
};