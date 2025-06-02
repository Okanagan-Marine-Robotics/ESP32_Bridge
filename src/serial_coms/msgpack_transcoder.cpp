#include <ArduinoJson.h>
#include <vector>
#include "configuration.h"

// Encode ArduinoJson document to MessagePack
std::vector<uint8_t> encodeToMsgPack(const JsonDocument &doc)
{

    // Check if the document is empty
    if (doc.isNull() || doc.size() == 0)
    {
        LOG_WEBSERIALLN("Document is empty, returning empty vector.");
    }

    // Estimate the required size and preallocate the buffer
    size_t estimatedSize = measureMsgPack(doc);
    std::vector<uint8_t> buffer;
    buffer.reserve(estimatedSize);

    // Use a lambda to write directly to the buffer
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
    serializeMsgPack(doc, writer);
    return buffer;
}

// Decode MessagePack data to ArduinoJson document
bool decodeFromMsgPack(const uint8_t *data, size_t size, JsonDocument &doc)
{
    DeserializationError error = deserializeMsgPack(doc, data, size);
    return !error;
}