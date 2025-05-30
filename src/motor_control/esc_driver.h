#pragma once

#include "driver/ledc.h"
#include "esp_err.h"
#include "configuration.h"

class ESCDriver
{
public:
    ESCDriver(int pwm_gpio, uint32_t freq_hz = ESC_PWM_FREQUENCY, int resolution_bits = ESC_PWM_RESOLUTION, int channel = 0);

    // Set ESC throttle: value in range [-1.0, 1.0] for bidirectional, or [0.0, 1.0] for unidirectional
    void setThrottle(float percent, bool bidirectional = ESC_BIDIRECTIONAL, float min_us = ESC_MIN, float max_us = ESC_MAX, float center_us = ESC_MID);

private:
    int pwm_gpio_;
    int channel_;
    int timer_;
    int freq_hz_;
    int resolution_bits_;
};