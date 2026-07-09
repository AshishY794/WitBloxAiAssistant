#pragma once

#include "config.h"
#include "servo_pwm.h"

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

// Moves mouth / eyes / hands / head while the device is speaking (TTS).
class TeacherAnimator {
public:
    TeacherAnimator();
    ~TeacherAnimator();

    void Start();
    void Stop();

private:
    static void TaskEntry(void* arg);
    void Run();
    void GoHome();
    void AnimateSpeaking(uint32_t tick_ms);

    ServoPwm servos_[TEACHER_SERVO_COUNT];
    TaskHandle_t task_ = nullptr;
    bool running_ = false;
    bool was_speaking_ = false;
};
