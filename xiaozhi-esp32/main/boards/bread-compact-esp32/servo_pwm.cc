#include "servo_pwm.h"

#include <algorithm>
#include <esp_log.h>

static const char* TAG = "ServoPwm";
static bool s_low_timer_configured = false;
static bool s_high_timer_configured = false;

static bool ConfigureServoTimer(ledc_mode_t speed_mode) {
    bool* configured = (speed_mode == LEDC_LOW_SPEED_MODE) ? &s_low_timer_configured
                                                             : &s_high_timer_configured;
    if (*configured) {
        return true;
    }

    ledc_timer_config_t timer = {
        .speed_mode = speed_mode,
        .duty_resolution = LEDC_TIMER_13_BIT,
        .timer_num = (speed_mode == LEDC_LOW_SPEED_MODE) ? LEDC_TIMER_1 : LEDC_TIMER_0,
        .freq_hz = 50,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    esp_err_t err = ledc_timer_config(&timer);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "ledc_timer_config mode=%d failed: %s", (int)speed_mode,
                 esp_err_to_name(err));
        return false;
    }
    *configured = true;
    return true;
}

bool ServoPwm::Attach(gpio_num_t pin, ledc_channel_t channel, ledc_mode_t speed_mode) {
    if (attached_) {
        Detach();
    }

    pin_ = pin;
    channel_ = channel;
    speed_mode_ = speed_mode;

    if (!ConfigureServoTimer(speed_mode_)) {
        return false;
    }

    ledc_timer_t timer_sel =
        (speed_mode_ == LEDC_LOW_SPEED_MODE) ? LEDC_TIMER_1 : LEDC_TIMER_0;

    ledc_channel_config_t ledc_channel = {
        .gpio_num = pin_,
        .speed_mode = speed_mode_,
        .channel = channel_,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = timer_sel,
        .duty = 0,
        .hpoint = 0,
    };
    esp_err_t err = ledc_channel_config(&ledc_channel);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "ledc_channel_config pin=%d ch=%d failed: %s", (int)pin_, (int)channel_,
                 esp_err_to_name(err));
        return false;
    }

    attached_ = true;
    // Do not force 90° here — caller sets the real home (0° arms were jumping to 90°)
    ESP_LOGI(TAG, "Servo attached pin=%d ch=%d mode=%s", (int)pin_, (int)channel_,
             speed_mode_ == LEDC_LOW_SPEED_MODE ? "low" : "high");
    return true;
}

void ServoPwm::Detach() {
    if (!attached_) {
        return;
    }
    ledc_stop(speed_mode_, channel_, 0);
    attached_ = false;
}

void ServoPwm::WriteAngle(int angle_deg) {
    if (!attached_) {
        return;
    }

    angle_ = std::clamp(angle_deg, 0, 180);
    // 0.5 ms .. 2.5 ms pulse inside 20 ms period, 13-bit duty (8191 max)
    uint32_t duty = (uint32_t)(((angle_ / 180.0) * 2.0 + 0.5) * 8191 / 20.0);
    ledc_set_duty(speed_mode_, channel_, duty);
    ledc_update_duty(speed_mode_, channel_);
}
