#pragma once
#include <ArduinoJson.h>
#include <vector>

// Encode ArduinoJson document to MessagePack
std::vector<uint8_t> encodeToMsgPack(const JsonDocument &doc);

// Decode MessagePack data to ArduinoJson document
bool decodeFromMsgPack(const uint8_t *data, size_t size, JsonDocument &doc);