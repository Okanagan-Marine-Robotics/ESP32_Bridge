#pragma once

#include "driver/ledc.h"
#include "esp_err.h"
#include "configuration.h"

class ESCDriver
{
public:
    ESCDriver(int pwm_gpio, int freq_hz = ESC_PWM_FREQUENCY, int resolution_bits = ESC_PWM_RESOLUTION, int channel = 0, int timer = 0);

    // Set ESC throttle: value in range [-1.0, 1.0] for bidirectional, or [0.0, 1.0] for unidirectional
    void setThrottle(float percent, bool bidirectional = false, float min_us = 1000.0f, float max_us = 2000.0f, float center_us = 1500.0f);

private:
    int pwm_gpio_;
    int channel_;
    int timer_;
    int freq_hz_;
    int resolution_bits_;
};