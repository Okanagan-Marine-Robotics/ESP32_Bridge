#include <Arduino.h>
#include "serial_io.h"
#include "msgpack_transcoder.h"
#include "crc8_calc.h"
#include "cobs_transcoder.h"

// Serial port configuration
#define ESP32_SERIAL Serial
#define ESP32_BAUDRATE 115200

void SerialIO::subscribe(uint8_t channel, Callback cb)
{
    _callbacks[channel] = cb;
}

void SerialIO::publish(int channel, const JsonDocument &doc)
{
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

    // print the encoded message for debugging in hex
    // Serial.print("Encoded message: ");
    // for (const auto &byte : encoded_message)
    // {
    //     Serial.print(byte, HEX);
    //     Serial.print(" ");
    // }
    // write the encoded message to the serial port
    write(encoded_message.data(), encoded_message.size());
    return;
}

void SerialIO::updateSubscriber()
{
    while (available())
    {
        int b = read();
        if (b < 0)
            break;

        uint8_t byte = static_cast<uint8_t>(b);
        if (byte == 0x00)
        {
            if (!_buffer.empty())
            {
                auto decoded = cobs_transcoder::decode(_buffer);
                _processPacket(decoded);
                _buffer.clear();
            }
        }
        else
        {
            _buffer.push_back(byte);
        }
    }
}

void SerialIO::_processPacket(const std::vector<uint8_t> &packet)
{
    if (packet.size() < 3)
    {
        Serial.println("Packet too short");
        return;
    }

    uint8_t received_crc = packet.back();
    std::vector<uint8_t> message(packet.begin(), packet.end() - 1);

    if (crc8(message.data(), message.size()) != received_crc)
    {
        Serial.println("CRC mismatch");
        return;
    }

    uint8_t channel = message[0];
    std::vector<uint8_t> payload(message.begin() + 1, message.end());

    JsonDocument doc; // Adjust size as needed

    if (!decodeFromMsgPack(payload.data(), payload.size(), doc))
    {
        Serial.println("MsgPack decoding failed");
        return;
    }

    auto it = _callbacks.find(channel);
    if (it != _callbacks.end())
    {
        it->second(doc);
    }
}

void SerialIO::begin()
{
    ESP32_SERIAL.begin(ESP32_BAUDRATE);
    while (!ESP32_SERIAL)
    {
        ; // Wait for serial port to connect. Needed for native USB
    }
    return;
}

size_t SerialIO::write(const uint8_t *buffer, size_t size)
{
    return ESP32_SERIAL.write(buffer, size);
}

int SerialIO::read()
{
    if (ESP32_SERIAL.available())
    {
        return ESP32_SERIAL.read();
    }
    return -1;
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