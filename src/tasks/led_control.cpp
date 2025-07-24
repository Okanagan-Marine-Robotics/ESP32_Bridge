#include "led_control.h"

void LedControl::setup()
{
    // Initialize the LED array
    FastLED.addLeds<WS2812, LED_PIN, GRB>(leds, NUM_LEDS);
    FastLED.setBrightness(50);

    // flash 3 times to indicate setup
    for (int i = 0; i < 3; ++i)
    {
        FastLED.showColor(CRGB::Green);
        delay(50);
        FastLED.clear();
        FastLED.show();
        delay(50);
    }

    // Start the LED blink task using static wrapper
    xTaskCreatePinnedToCore(
        ledBlinkTaskWrapper,
        "ledBlinkTask",
        2048,
        this,
        1,
        nullptr,
        1);
}

void LedControl::ledBlinkTaskWrapper(void *parameter)
{
    LedControl *instance = static_cast<LedControl *>(parameter);
    instance->ledBlinkTask();
}

void LedControl::ledBlinkTask()
{
    for (;;)
    {
        // Blink the LEDs
        FastLED.showColor(CRGB::Red);
        vTaskDelay(pdMS_TO_TICKS(500));
        FastLED.clear();
        FastLED.show();
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}
