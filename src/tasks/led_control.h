#pragma once
#include <Arduino.h>
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
