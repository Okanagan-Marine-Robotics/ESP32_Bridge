#pragma once
#include <Arduino.h>

// CRC parameters
#define CRC8_INIT 0x00 // Default initial value for SMBus
#define CRC8_POLY 0x07 // Default polynomial for SMBus: x^8 + x^2 + x^1 + 1

uint8_t crc8(const uint8_t *data, size_t len);