#include <ArduinoJson.h>
#include <vector>

// Encode ArduinoJson document to MessagePack
std::vector<uint8_t> encodeToMsgPack(const JsonDocument &doc)
{
    std::vector<uint8_t> buffer;
    buffer.reserve(256); // Reserve initial space to avoid reallocations
                         // Reserve some space to avoid reallocations
    struct VectorWriter : public Print
    {
        std::vector<uint8_t> &buffer;

        VectorWriter(std::vector<uint8_t> &buf) : buffer(buf) {}

        size_t write(uint8_t byte) override
        {
            buffer.push_back(byte);
            return 1;
        }

        size_t write(const uint8_t *data, size_t size) override
        {
            buffer.insert(buffer.end(), data, data + size);
            return size;
        }
    };

    VectorWriter writer(buffer);
    // Serialize the JsonDocument to MessagePack format into the buffer
    serializeMsgPack(doc, writer);
    return buffer;
}

// Decode MessagePack data to ArduinoJson document
bool decodeFromMsgPack(const uint8_t *data, size_t size, JsonDocument &doc)
{
    DeserializationError error = deserializeMsgPack(doc, data, size);
    return !error;
}