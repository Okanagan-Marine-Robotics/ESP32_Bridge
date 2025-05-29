#include "esc_driver.h"

ESCDriver::ESCDriver(int pwm_gpio, int freq_hz, int resolution_bits, int channel, int timer)
    : pwm_gpio_(pwm_gpio), channel_(channel), timer_(timer), freq_hz_(freq_hz), resolution_bits_(resolution_bits)
{
    ledc_timer_config_t ledc_timer = {
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .duty_resolution = (ledc_timer_bit_t)resolution_bits_,
        .timer_num = (ledc_timer_t)timer_,
        .freq_hz = freq_hz_,
        .clk_cfg = LEDC_AUTO_CLK};
    ledc_timer_config(&ledc_timer);

    ledc_channel_config_t ledc_channel = {
        .gpio_num = pwm_gpio_,
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .channel = (ledc_channel_t)channel_,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = (ledc_timer_t)timer_,
        .duty = 0,
        .hpoint = 0};
    ledc_channel_config(&ledc_channel);
}

void ESCDriver::setThrottle(float percent, bool bidirectional, float min_us, float max_us, float center_us)
{
    float us;
    if (bidirectional)
    {
        percent = (percent < -1.0f) ? -1.0f : (percent > 1.0f ? 1.0f : percent);
        if (percent >= 0.0f)
        {
            us = center_us + (max_us - center_us) * percent;
        }
        else
        {
            us = center_us + (center_us - min_us) * percent;
        }
    }
    else
    {
        percent = (percent < 0.0f) ? 0.0f : (percent > 1.0f ? 1.0f : percent);
        us = min_us + (max_us - min_us) * percent;
    }
    uint32_t max_duty = (1 << resolution_bits_) - 1;
    uint32_t duty = static_cast<uint32_t>((us * max_duty) / 20000.0f);
    ledc_set_duty(LEDC_HIGH_SPEED_MODE, (ledc_channel_t)channel_, duty);
    ledc_update_duty(LEDC_HIGH_SPEED_MODE, (ledc_channel_t)channel_);
}
