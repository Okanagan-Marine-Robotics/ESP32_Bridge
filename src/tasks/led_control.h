#pragma once
#include <Arduino.h>
#define FASTLED_INTERRUPT_RETRY_COUNT 3
#include <FastLED.h>
#include "configuration.h"

class LedControl
{
public:
    void setup();

private:
    static void ledBlinkTaskWrapper(void *parameter);
    void ledBlinkTask();
    CRGB leds[NUM_LEDS]; // Array to hold LED colors
};
