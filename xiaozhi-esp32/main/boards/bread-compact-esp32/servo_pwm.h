#pragma once

#include "driver/gpio.h"
#include "driver/ledc.h"

// Simple hobby-servo driver using ESP32 LEDC (50 Hz PWM).
class ServoPwm {
public:
    ServoPwm() = default;

  // speed_mode: LEDC_LOW_SPEED_MODE for channels 0–7, LEDC_HIGH_SPEED_MODE for extra channels 0–3
    bool Attach(gpio_num_t pin, ledc_channel_t channel,
                ledc_mode_t speed_mode = LEDC_LOW_SPEED_MODE);
    void Detach();
    void WriteAngle(int angle_deg);  // 0..180
    int GetAngle() const { return angle_; }
    bool IsAttached() const { return attached_; }

private:
    bool attached_ = false;
    gpio_num_t pin_ = GPIO_NUM_NC;
    ledc_channel_t channel_ = LEDC_CHANNEL_0;
    ledc_mode_t speed_mode_ = LEDC_LOW_SPEED_MODE;
    int angle_ = 90;
};
