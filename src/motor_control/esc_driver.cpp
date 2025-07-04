#include "esc_driver.h"
#include <MycilaWebSerial.h>

ESCDriver::ESCDriver(int pwm_gpio, uint32_t freq_hz, int resolution_bits, int channel)
    : pwm_gpio_(pwm_gpio), channel_(channel), freq_hz_(freq_hz), resolution_bits_(resolution_bits)
{
    // Setup the channel with desired frequency and resolution
    ledcSetup(channel_, freq_hz_, resolution_bits_);

    // Attach the GPIO to the channel
    ledcAttachPin(pwm_gpio_, channel_);
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
    uint32_t duty = static_cast<uint32_t>((us * max_duty * freq_hz_) / 1000000.0f);
    ledcWrite(channel_, duty);
}

void ESCDriver::setDutyUs(uint32_t set_us)
{
    // Ensure the set_us is within the valid range
    if (set_us < ESC_MIN || set_us > ESC_MAX)
    {
        LOG_WEBSERIALLN("setDutyUs: Value out of range, must be between ESC_MIN and ESC_MAX");
        return;
    }

    // Convert microseconds to duty cycle
    uint32_t max_duty = (1 << resolution_bits_) - 1;
    uint32_t duty = static_cast<uint32_t>((set_us * max_duty * freq_hz_) / 1000000.0f);
    ledcWrite(channel_, duty);
}
