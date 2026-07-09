#include "teacher_animator.h"

#include "application.h"
#include "device_state.h"
#include "teacher_servo_config.h"

#include <algorithm>
#include <cmath>
#include <esp_log.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static const char* TAG = "TeacherAnimator";

static int ClampAngle(int idx, int angle) {
    const auto& cfg = kTeacherServos[idx];
    return std::clamp(angle, cfg.min_deg, cfg.max_deg);
}

TeacherAnimator::TeacherAnimator() = default;

TeacherAnimator::~TeacherAnimator() {
    Stop();
    for (int i = 0; i < TEACHER_SERVO_COUNT; ++i) {
        servos_[i].Detach();
    }
}

void TeacherAnimator::Start() {
    if (running_) {
        return;
    }

    for (int i = 0; i < TEACHER_SERVO_COUNT; ++i) {
        const auto& cfg = kTeacherServos[i];
        if (!servos_[i].Attach(cfg.pin, cfg.ledc_ch, cfg.speed_mode)) {
            ESP_LOGE(TAG, "Failed to attach %s on GPIO %d", cfg.name, (int)cfg.pin);
        } else {
            ESP_LOGI(TAG, "Servo %s: GPIO %d, home %d deg", cfg.name, (int)cfg.pin, cfg.home_deg);
        }
    }

    GoHome();
    running_ = true;
    was_speaking_ = false;

    xTaskCreate(TaskEntry, "teacher_servo", TEACHER_SERVO_TASK_STACK, this,
                TEACHER_SERVO_TASK_PRIORITY, &task_);
    ESP_LOGI(TAG, "Teacher animator started (%d servos)", TEACHER_SERVO_COUNT);
}

void TeacherAnimator::Stop() {
    running_ = false;
    if (task_ != nullptr) {
        for (int i = 0; i < 50 && task_ != nullptr; ++i) {
            vTaskDelay(pdMS_TO_TICKS(20));
        }
        if (task_ != nullptr) {
            vTaskDelete(task_);
            task_ = nullptr;
        }
    }
    GoHome();
}

void TeacherAnimator::TaskEntry(void* arg) {
    static_cast<TeacherAnimator*>(arg)->Run();
}

void TeacherAnimator::Run() {
    while (running_) {
        const bool speaking =
            Application::GetInstance().GetDeviceState() == kDeviceStateSpeaking;

        if (speaking) {
            AnimateSpeaking(xTaskGetTickCount() * portTICK_PERIOD_MS);
            was_speaking_ = true;
        } else if (was_speaking_) {
            GoHome();
            was_speaking_ = false;
            ESP_LOGI(TAG, "Speech ended -> home pose");
        }

        vTaskDelay(pdMS_TO_TICKS(TEACHER_SERVO_UPDATE_MS));
    }

    task_ = nullptr;
    vTaskDelete(nullptr);
}

void TeacherAnimator::GoHome() {
    for (int i = 0; i < TEACHER_SERVO_COUNT; ++i) {
        if (servos_[i].IsAttached()) {
            servos_[i].WriteAngle(kTeacherServos[i].home_deg);
        }
    }
}

void TeacherAnimator::AnimateSpeaking(uint32_t tick_ms) {
    const float t = tick_ms / 1000.0f;
    const bool blinking =
        (tick_ms % TEACHER_EYE_BLINK_INTERVAL_MS) < TEACHER_EYE_BLINK_HOLD_MS;

    for (int i = 0; i < TEACHER_SERVO_COUNT; ++i) {
        if (!servos_[i].IsAttached()) {
            continue;
        }

        const auto& cfg = kTeacherServos[i];
        int angle = cfg.home_deg;

        if (cfg.blink_on_speak && blinking) {
            angle = TEACHER_EYE_BLINK_CLOSED_DEG;
        } else if (cfg.speak_amp_deg > 0.0f) {
            const float wave = std::sin(2.0f * static_cast<float>(M_PI) * cfg.speak_freq_hz * t
                                        + cfg.speak_phase_rad);
            angle = cfg.home_deg + static_cast<int>(cfg.speak_amp_deg * wave);
        }

        servos_[i].WriteAngle(ClampAngle(i, angle));
    }
}
