#include <Arduino.h>
#include "serial_io.h"
#include "msgpack_transcoder.h"
#include "crc8_calc.h"
#include "cobs_transcoder.h"
#include "MycilaWebSerial.h"
#include "configuration.h"
#include "driver/uart.h"

SerialIO serialio;

void serialTask(void *parameter)
{
    for (;;)
    {
        serialio.updateSubscribers(); // Process incoming serial data
        vTaskDelay(pdMS_TO_TICKS(1)); // Delay to prevent busy-waiting
    }
}

void SerialIO::subscribe(uint8_t channel, Callback cb)
{
    _callbacks[channel] = cb;
}

void SerialIO::publish(int channel, const JsonDocument &doc)
{

    // why do we sometimes get empty documents?
    // if (doc.isNull() || doc.size() == 0)
    // {
    //     LOG_WEBSERIALLN("Warning: Tried to publish empty JsonDocument");
    //     return;
    // }

    digitalWrite(LED_PIN, HIGH); // Turn on the LED to indicate activity
    auto msgpackdata = encodeToMsgPack(doc);
    // add channel information to the beginning of the message
    std::vector<uint8_t> message;

    message.push_back(static_cast<uint8_t>(channel));
    message.insert(message.end(), msgpackdata.begin(), msgpackdata.end());

    // calculate CRC8 for the message
    uint8_t crc = crc8(message.data(), message.size());

    // append CRC8 to the end of the message
    message.push_back(crc);

    // encode the message using COBS
    auto encoded_message = cobs_transcoder::encode(message);

    // append 0x00 byte to the end of the encoded message
    encoded_message.push_back(0x00);

    // write the encoded message to the serial port
    write(encoded_message.data(), encoded_message.size());
    digitalWrite(LED_PIN, LOW); // Turn off the LED after sending
    return;
}

void SerialIO::_processPacket(const std::vector<uint8_t> &packet)
{
    if (packet.size() < 3)
    {
        LOG_WEBSERIALLN("Packet too short");
        return;
    }

    uint8_t received_crc = packet.back();
    std::vector<uint8_t> message(packet.begin(), packet.end() - 1);

    if (crc8(message.data(), message.size()) != received_crc)
    {
        LOG_WEBSERIALLN("CRC mismatch");
        return;
    }

    uint8_t channel = message[0];
    std::vector<uint8_t> payload(message.begin() + 1, message.end());

    JsonDocument doc; // Adjust size as needed
    doc.clear();      // Clear the document to avoid residual data

    if (!decodeFromMsgPack(payload.data(), payload.size(), doc))
    {
        LOG_WEBSERIALLN("MsgPack decoding failed");
        return;
    }

    auto it = _callbacks.find(channel);
    if (it != _callbacks.end())
    {
        it->second(doc);
        return;
    }
    return;
}

void SerialIO::begin()
{
    pinMode(LED_PIN, OUTPUT);

    ESP32_SERIAL.begin(ESP32_BAUDRATE);
    ESP32_SERIAL.onReceive([this]()
                           { this->onUartRx(); }); // Register the ISR callback as lambda
}

size_t SerialIO::write(const uint8_t *buffer, size_t size)
{
    auto ret = ESP32_SERIAL.write(buffer, size);
    if (ret != size)
    {
        LOG_WEBSERIALLN("Warning: Not all bytes written!");
    }
    ESP32_SERIAL.flush();
    return ret;
}

size_t SerialIO::readBytes(uint8_t *buffer, size_t length)
{
    return ESP32_SERIAL.readBytes(buffer, length);
}

bool SerialIO::available()
{
    return ESP32_SERIAL.available() > 0;
}

void SerialIO::flush()
{
    ESP32_SERIAL.flush();
    return;
}

void SerialIO::updateSubscribers()
{
    uint8_t byte;
    while (_rxRing.pop(byte))
    {
        if (byte == 0x00)
        {
            if (!_buffer.empty())
            {
                digitalWrite(LED_PIN, HIGH); // Turn on the LED to indicate activity
                auto decoded = cobs_transcoder::decode(_buffer);
                _processPacket(decoded);
                _buffer.clear();
                digitalWrite(LED_PIN, LOW); // Turn off the LED after processing
            }
        }
        else
        {
            _buffer.push_back(byte);
            if (_buffer.size() > MAX_SERIAL_BUFFER_SIZE)
            {
                LOG_WEBSERIALLN("Buffer overflow, clearing buffer");
                _buffer.clear();
            }
        }
    }
}

void SerialIO::onUartRx()
{
    while (ESP32_SERIAL.available())
    {
        uint8_t byte = ESP32_SERIAL.read();
        if (!_rxRing.push(byte))
        {
            LOG_WEBSERIALLN("Ring buffer overflow");
        }
    }
}