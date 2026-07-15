#pragma once

#include "config.h"
#include "servo_pwm.h"

#include <atomic>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

// Talk animation (audio-synced) + arm gestures the hardware can actually do.
// Hardware: 2 arms that lift from home (not palms meeting / not full-body bow).
class TeacherAnimator {
public:
    // Only gestures that look correct with left/right lift arms.
    enum class Gesture {
        None = 0,
        Handshake,  // offer + shake + alternate raises
        Wave,       // one-arm wave
        HighFive,   // one arm up
        Cheer,      // both arms up / bounce
        Applause,   // both arms bounce (not palm-clap)
        Dance,      // alternate left/right
        Yes,        // both arms pump
        No,         // left/right alternate
    };

    TeacherAnimator();
    ~TeacherAnimator();

    void Start();
    void Stop();

    void StartGesture(Gesture g);
    bool IsGestureRunning() const { return gesture_active_.load(); }

    void StartHandshake() { StartGesture(Gesture::Handshake); }
    bool IsHandshakeRunning() const { return IsGestureRunning(); }

private:
    enum class ActiveHand { None, Left, Right };

    static void TaskEntry(void* arg);
    void Run();
    void GoHome();
    void EaseTowardHome();
    void ResetSpeakState(uint32_t now_ms);
    void AnimateSpeaking(uint32_t now_ms, float audio_energy);
    void UpdateEyes(uint32_t now_ms);
    void UpdateMouth(float energy);
    void UpdateHands(uint32_t now_ms, float energy);
    void UpdateGesture(uint32_t now_ms);
    void WriteSmoothed(int idx, float target_deg, float alpha);
    float ArmAngle(int idx, float strength) const;
    void SetArms(float left, float right, float alpha);
    void RegisterMcpTools();
    void FinishGesture();

    ServoPwm servos_[TEACHER_SERVO_COUNT];
    float current_deg_[TEACHER_SERVO_COUNT] = {};
    TaskHandle_t task_ = nullptr;
    bool running_ = false;
    bool was_speaking_ = false;
    bool audio_was_playing_ = false;

    uint32_t next_eye_ms_ = 0;
    uint32_t next_hand_ms_ = 0;
    int eye_target_ = 90;
    ActiveHand active_hand_ = ActiveHand::Left;
    float mouth_env_ = 0.0f;
    float hand_env_ = 0.0f;

    std::atomic<bool> gesture_active_{false};
    Gesture gesture_ = Gesture::None;
    int g_step_ = 0;
    uint32_t g_step_t0_ms_ = 0;
    int g_count_ = 0;
};
