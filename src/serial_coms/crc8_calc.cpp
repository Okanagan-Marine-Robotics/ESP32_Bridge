#include <Arduino.h>
#include "crc8_calc.h"

// Compute CRC-8 with configurable init and poly
uint8_t crc8(const uint8_t *data, size_t len)
{
    uint8_t crc = CRC8_INIT;

    for (size_t i = 0; i < len; ++i)
    {
        crc ^= data[i];
        for (uint8_t bit = 0; bit < 8; ++bit)
        {
            if (crc & 0x80)
                crc = (crc << 1) ^ CRC8_POLY;
            else
                crc <<= 1;
        }
    }

    return crc;
}