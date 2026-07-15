#include "teacher_animator.h"

#include "application.h"
#include "device_state.h"
#include "mcp_server.h"
#include "teacher_servo_config.h"

#include <algorithm>
#include <cmath>
#include <esp_log.h>
#include <esp_random.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static const char* TAG = "TeacherAnimator";

namespace {

int ClampDeg(int idx, int angle) {
    const auto& c = kTeacherServos[idx];
    return std::clamp(angle, c.min_deg, c.max_deg);
}

uint32_t RandMs(uint32_t lo, uint32_t hi) {
    return lo + (esp_random() % (hi - lo + 1));
}

float Lerp(float a, float b, float t) {
    return a + (b - a) * t;
}

}  // namespace

TeacherAnimator::TeacherAnimator() = default;

TeacherAnimator::~TeacherAnimator() {
    Stop();
    for (int i = 0; i < TEACHER_SERVO_COUNT; ++i) {
        servos_[i].Detach();
    }
}

void TeacherAnimator::RegisterMcpTools() {
    auto& mcp = McpServer::GetInstance();

    // Only tools the 2 lift-arms can actually perform well.
    mcp.AddTool(
        "self.teacher.shake_hands",
        "Handshake show: reach one arm, shake it, then raise left/right alternately. "
        "Call for: shake hands, handshake, हाथ मिलाओ. "
        "Arms lift only — they cannot bring palms together.",
        PropertyList(),
        [this](const PropertyList&) -> ReturnValue {
            StartGesture(Gesture::Handshake);
            return "Handshake gesture started";
        });

    mcp.AddTool(
        "self.teacher.wave",
        "Wave one arm (hello/bye). Call for: hello, hi, bye, wave, नमस्ते, अलविदा.",
        PropertyList(),
        [this](const PropertyList&) -> ReturnValue {
            StartGesture(Gesture::Wave);
            return "Wave started";
        });

    mcp.AddTool(
        "self.teacher.high_five",
        "Raise one arm up (high-five pose). Call for: high five, give me five.",
        PropertyList(),
        [this](const PropertyList&) -> ReturnValue {
            StartGesture(Gesture::HighFive);
            return "High five started";
        });

    mcp.AddTool(
        "self.teacher.cheer",
        "Both arms up celebrating. Call for: cheer, celebrate, well done, yay, शाबाश.",
        PropertyList(),
        [this](const PropertyList&) -> ReturnValue {
            StartGesture(Gesture::Cheer);
            return "Cheer started";
        });

    mcp.AddTool(
        "self.teacher.applause",
        "Applause: both arms bounce up and down. Call for: clap, applause, ताली. "
        "Not a real palm-clap — robot arms cannot meet in the middle.",
        PropertyList(),
        [this](const PropertyList&) -> ReturnValue {
            StartGesture(Gesture::Applause);
            return "Applause started";
        });

    mcp.AddTool(
        "self.teacher.dance",
        "Fun dance: left and right arms alternate. Call for: dance, नाचो.",
        PropertyList(),
        [this](const PropertyList&) -> ReturnValue {
            StartGesture(Gesture::Dance);
            return "Dance started";
        });

    mcp.AddTool(
        "self.teacher.say_yes",
        "Yes: both arms pump up-down twice. Call for: show yes, say yes with hands.",
        PropertyList(),
        [this](const PropertyList&) -> ReturnValue {
            StartGesture(Gesture::Yes);
            return "Yes gesture started";
        });

    mcp.AddTool(
        "self.teacher.say_no",
        "No: left arm then right arm alternately. Call for: show no, say no with hands.",
        PropertyList(),
        [this](const PropertyList&) -> ReturnValue {
            StartGesture(Gesture::No);
            return "No gesture started";
        });
}

void TeacherAnimator::StartGesture(Gesture g) {
    if (g == Gesture::None) {
        return;
    }
    gesture_ = g;
    g_step_ = 0;
    g_count_ = 0;
    g_step_t0_ms_ = xTaskGetTickCount() * portTICK_PERIOD_MS;
    gesture_active_.store(true);
    ESP_LOGI(TAG, "gesture start id=%d", static_cast<int>(g));
}

void TeacherAnimator::FinishGesture() {
    GoHome();
    gesture_active_.store(false);
    gesture_ = Gesture::None;
    g_step_ = 0;
    g_count_ = 0;
    ESP_LOGI(TAG, "gesture finished");
}

void TeacherAnimator::Start() {
    if (running_) {
        return;
    }

    for (int i = 0; i < TEACHER_SERVO_COUNT; ++i) {
        const auto& c = kTeacherServos[i];
        if (!servos_[i].Attach(c.pin, c.ledc_ch, c.speed_mode)) {
            ESP_LOGE(TAG, "attach failed %s GPIO %d", c.name, (int)c.pin);
            continue;
        }
        current_deg_[i] = static_cast<float>(c.home_deg);
        servos_[i].WriteAngle(c.home_deg);
        ESP_LOGI(TAG, "%s GPIO %d home %d", c.name, (int)c.pin, c.home_deg);
    }

    GoHome();
    RegisterMcpTools();
    running_ = true;
    was_speaking_ = false;
    audio_was_playing_ = false;
    gesture_active_.store(false);

    xTaskCreate(TaskEntry, "teacher_servo", TEACHER_SERVO_TASK_STACK, this,
                TEACHER_SERVO_TASK_PRIORITY, &task_);
    ESP_LOGI(TAG, "started (honest lift-arm gestures only)");
}

void TeacherAnimator::Stop() {
    running_ = false;
    gesture_active_.store(false);
    for (int i = 0; i < 50 && task_ != nullptr; ++i) {
        vTaskDelay(pdMS_TO_TICKS(20));
    }
    if (task_ != nullptr) {
        vTaskDelete(task_);
        task_ = nullptr;
    }
    GoHome();
}

void TeacherAnimator::TaskEntry(void* arg) {
    static_cast<TeacherAnimator*>(arg)->Run();
}

void TeacherAnimator::Run() {
    while (running_) {
        auto& app = Application::GetInstance();
        // Drive servos from real PCM playback (cloud TTS or local greeting.ogg)
        const bool speaking = app.GetDeviceState() == kDeviceStateSpeaking;
        const bool audio_on = app.GetAudioService().IsPlayingAudio(150);
        const float energy = app.GetAudioService().GetPlaybackEnergy();
        const uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;

        if (gesture_active_.load()) {
            UpdateGesture(now);
            if (audio_on) {
                UpdateMouth(energy);
                UpdateEyes(now);
            }
        } else if (audio_on) {
            if (!audio_was_playing_) {
                ResetSpeakState(now);
            }
            AnimateSpeaking(now, energy);
            audio_was_playing_ = true;
            was_speaking_ = true;
        } else if (speaking && audio_was_playing_) {
            EaseTowardHome();
            was_speaking_ = true;
        } else if (speaking) {
            was_speaking_ = true;
        } else if (was_speaking_) {
            GoHome();
            was_speaking_ = false;
            audio_was_playing_ = false;
        }

        vTaskDelay(pdMS_TO_TICKS(TEACHER_SERVO_UPDATE_MS));
    }

    task_ = nullptr;
    vTaskDelete(nullptr);
}

void TeacherAnimator::GoHome() {
    for (int i = 0; i < TEACHER_SERVO_COUNT; ++i) {
        if (!servos_[i].IsAttached()) {
            continue;
        }
        const int home = kTeacherServos[i].home_deg;
        current_deg_[i] = static_cast<float>(home);
        servos_[i].WriteAngle(home);
    }
    mouth_env_ = 0.0f;
    hand_env_ = 0.0f;
}

void TeacherAnimator::EaseTowardHome() {
    for (int i = 0; i < TEACHER_SERVO_COUNT; ++i) {
        if (i == SERVO_EYES) {
            continue;
        }
        WriteSmoothed(i, static_cast<float>(kTeacherServos[i].home_deg), 0.18f);
    }
    mouth_env_ = Lerp(mouth_env_, 0.0f, 0.35f);
    hand_env_ = Lerp(hand_env_, 0.0f, 0.20f);
}

void TeacherAnimator::ResetSpeakState(uint32_t now_ms) {
    next_eye_ms_ = now_ms + RandMs(800, 1500);
    next_hand_ms_ = now_ms + RandMs(400, 900);
    eye_target_ = TEACHER_EYE_LOOK_CENTER_DEG;
    active_hand_ = (esp_random() & 1) ? ActiveHand::Left : ActiveHand::Right;
    mouth_env_ = 0.0f;
    hand_env_ = 0.0f;
}

void TeacherAnimator::WriteSmoothed(int idx, float target_deg, float alpha) {
    if (!servos_[idx].IsAttached()) {
        return;
    }
    current_deg_[idx] = Lerp(current_deg_[idx], target_deg, alpha);
    servos_[idx].WriteAngle(ClampDeg(idx, static_cast<int>(std::lround(current_deg_[idx]))));
}

float TeacherAnimator::ArmAngle(int idx, float strength) const {
    const auto& c = kTeacherServos[idx];
    return c.home_deg + c.speak_amp_deg * std::clamp(strength, 0.0f, 1.0f);
}

void TeacherAnimator::SetArms(float left, float right, float alpha) {
    WriteSmoothed(SERVO_SHOULDER_L, ArmAngle(SERVO_SHOULDER_L, left), alpha);
    WriteSmoothed(SERVO_ELBOW_L, ArmAngle(SERVO_ELBOW_L, left * 0.95f), alpha);
    WriteSmoothed(SERVO_SHOULDER_R, ArmAngle(SERVO_SHOULDER_R, right), alpha);
    WriteSmoothed(SERVO_ELBOW_R, ArmAngle(SERVO_ELBOW_R, right * 0.95f), alpha);
}

void TeacherAnimator::UpdateMouth(float energy) {
    const float gated = (energy < 0.08f) ? 0.0f : energy;
    mouth_env_ = Lerp(mouth_env_, gated, 0.55f);
    const auto& m = kTeacherServos[SERVO_MOUTH];
    WriteSmoothed(SERVO_MOUTH, m.home_deg + m.speak_amp_deg * mouth_env_, 0.50f);
}

void TeacherAnimator::UpdateEyes(uint32_t now_ms) {
    if (now_ms >= next_eye_ms_) {
        const int pick = static_cast<int>(esp_random() % 5);
        if (pick == 0) {
            eye_target_ = TEACHER_EYE_LOOK_LEFT_DEG;
        } else if (pick == 1) {
            eye_target_ = TEACHER_EYE_LOOK_RIGHT_DEG;
        } else {
            eye_target_ = TEACHER_EYE_LOOK_CENTER_DEG;
        }
        next_eye_ms_ = now_ms + RandMs(1000, 2400);
    }
    WriteSmoothed(SERVO_EYES, static_cast<float>(eye_target_), 0.10f);
}

void TeacherAnimator::UpdateHands(uint32_t now_ms, float energy) {
    const float gated = (energy < 0.08f) ? 0.0f : energy;
    hand_env_ = Lerp(hand_env_, gated, 0.26f);

    if (now_ms >= next_hand_ms_) {
        const int roll = static_cast<int>(esp_random() % 100);
        if (roll < 12) {
            active_hand_ = ActiveHand::None;
            next_hand_ms_ = now_ms + RandMs(400, 800);
        } else if (active_hand_ == ActiveHand::Left) {
            active_hand_ = ActiveHand::Right;
            next_hand_ms_ = now_ms + RandMs(1100, 2200);
        } else if (active_hand_ == ActiveHand::Right) {
            active_hand_ = ActiveHand::Left;
            next_hand_ms_ = now_ms + RandMs(1100, 2200);
        } else {
            active_hand_ = (esp_random() & 1) ? ActiveHand::Left : ActiveHand::Right;
            next_hand_ms_ = now_ms + RandMs(900, 1800);
        }
    }

    float left = 0.0f;
    float right = 0.0f;
    if (active_hand_ == ActiveHand::Left) {
        left = hand_env_;
    } else if (active_hand_ == ActiveHand::Right) {
        right = hand_env_;
    }
    SetArms(left, right, 0.20f);
}

void TeacherAnimator::UpdateGesture(uint32_t now_ms) {
    const uint32_t elapsed = now_ms - g_step_t0_ms_;
    constexpr float kA = 0.22f;

    auto next_step = [&](uint32_t hold_ms) {
        if (elapsed >= hold_ms) {
            g_step_++;
            g_step_t0_ms_ = now_ms;
            g_count_ = 0;
            return true;
        }
        return false;
    };

    switch (gesture_) {
        case Gesture::Handshake:
            if (g_step_ == 0) {
                SetArms(0.0f, 0.85f, kA);
                next_step(700);
            } else if (g_step_ == 1) {
                SetArms(0.0f, 0.55f + 0.35f * std::sin(elapsed / 90.0f), 0.35f);
                next_step(1400);
            } else if (g_step_ == 2) {
                SetArms(1.0f, 0.0f, kA);
                next_step(700);
            } else if (g_step_ == 3) {
                SetArms(0.0f, 1.0f, kA);
                next_step(700);
            } else if (g_step_ == 4) {
                SetArms(1.0f, 1.0f, kA);
                next_step(900);
            } else if (g_step_ == 5) {
                SetArms((g_count_ % 2) == 0 ? 1.0f : 0.0f, (g_count_ % 2) ? 1.0f : 0.0f, kA);
                if (elapsed >= 550) {
                    g_count_++;
                    g_step_t0_ms_ = now_ms;
                    if (g_count_ >= 4) {
                        g_step_ = 6;
                        g_step_t0_ms_ = now_ms;
                    }
                }
            } else {
                SetArms(0.0f, 0.0f, 0.18f);
                if (elapsed >= 600) {
                    FinishGesture();
                }
            }
            break;

        case Gesture::Wave:
            if (g_step_ == 0) {
                SetArms(0.0f, 0.9f, kA);
                next_step(400);
            } else if (g_step_ < 5) {
                SetArms(0.0f, 0.45f + 0.45f * std::sin(elapsed / 110.0f), 0.35f);
                next_step(900);
            } else {
                SetArms(0.0f, 0.0f, 0.18f);
                if (elapsed >= 500) {
                    FinishGesture();
                }
            }
            break;

        case Gesture::HighFive:
            if (g_step_ == 0) {
                SetArms(0.0f, 1.0f, kA);
                next_step(1200);
            } else if (g_step_ == 1) {
                SetArms(0.0f, 0.7f, kA);
                next_step(500);
            } else {
                SetArms(0.0f, 0.0f, 0.18f);
                if (elapsed >= 500) {
                    FinishGesture();
                }
            }
            break;

        case Gesture::Cheer:
            if (g_step_ == 0) {
                SetArms(1.0f, 1.0f, kA);
                next_step(500);
            } else if (g_step_ < 4) {
                const float b = 0.65f + 0.35f * std::sin(elapsed / 140.0f);
                SetArms(b, b, 0.30f);
                next_step(800);
            } else {
                SetArms(0.0f, 0.0f, 0.18f);
                if (elapsed >= 500) {
                    FinishGesture();
                }
            }
            break;

        case Gesture::Applause:
            // Honest: both arms bounce — not palms meeting
            if (g_step_ < 8) {
                const bool up = (g_step_ % 2) == 0;
                SetArms(up ? 1.0f : 0.15f, up ? 1.0f : 0.15f, 0.40f);
                next_step(280);
            } else {
                SetArms(0.0f, 0.0f, 0.18f);
                if (elapsed >= 450) {
                    FinishGesture();
                }
            }
            break;

        case Gesture::Dance:
            if (g_step_ < 8) {
                const float ph = elapsed / 160.0f;
                SetArms(0.4f + 0.55f * std::max(0.0f, std::sin(ph)),
                        0.4f + 0.55f * std::max(0.0f, std::sin(ph + static_cast<float>(M_PI))),
                        0.30f);
                next_step(700);
            } else {
                SetArms(0.0f, 0.0f, 0.18f);
                if (elapsed >= 500) {
                    FinishGesture();
                }
            }
            break;

        case Gesture::Yes:
            if (g_step_ < 4) {
                const bool up = (g_step_ % 2) == 0;
                SetArms(up ? 0.9f : 0.2f, up ? 0.9f : 0.2f, 0.35f);
                next_step(350);
            } else {
                SetArms(0.0f, 0.0f, 0.18f);
                if (elapsed >= 400) {
                    FinishGesture();
                }
            }
            break;

        case Gesture::No:
            if (g_step_ < 6) {
                const bool left = (g_step_ % 2) == 0;
                SetArms(left ? 0.85f : 0.0f, left ? 0.0f : 0.85f, 0.35f);
                next_step(320);
            } else {
                SetArms(0.0f, 0.0f, 0.18f);
                if (elapsed >= 400) {
                    FinishGesture();
                }
            }
            break;

        default:
            FinishGesture();
            break;
    }
}

void TeacherAnimator::AnimateSpeaking(uint32_t now_ms, float audio_energy) {
    UpdateMouth(audio_energy);
    UpdateEyes(now_ms);
    UpdateHands(now_ms, audio_energy);
}
