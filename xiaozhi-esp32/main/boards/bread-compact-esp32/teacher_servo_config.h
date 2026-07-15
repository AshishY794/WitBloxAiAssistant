#pragma once

// =============================================================================
// TEACHER ROBOT — SINGLE SERVO CONFIG FILE (edit only this file)
// =============================================================================
//
// Servo layout (6 active):
//   • 1 mouth
//   • 1 eyes (both eyes on ONE servo)
//   • 4 arms — shoulder + elbow per arm
//
// When WitBlox AI speaks (TTS), servos animate. When speech stops → home pose.
//
// Hardware wiring (each servo has 3 wires):
//   - Orange/Yellow → GPIO pin listed below (signal)
//   - Red           → external 5V power supply (NOT ESP32 3.3V)
//   - Brown/Black   → GND (common with ESP32 GND)
//
// After changing pins or angles: rebuild and flash firmware.
//
// =============================================================================

#include <driver/gpio.h>
#include <driver/ledc.h>

// -----------------------------------------------------------------------------
// HOW MANY SERVOS
// -----------------------------------------------------------------------------
#define TEACHER_SERVO_COUNT 6

#define SERVO_COUNT TEACHER_SERVO_COUNT

// Index numbers for kTeacherServos[] — order must match table below
#define SERVO_MOUTH       0  // jaw
#define SERVO_EYES        1  // both eyes — one servo (look L/C/R)
#define SERVO_SHOULDER_L  2
#define SERVO_ELBOW_L     3
#define SERVO_SHOULDER_R  4
#define SERVO_ELBOW_R     5

// Keep old names so other code still compiles
#define SERVO_EYE_L       SERVO_EYES
#define SERVO_EYE_R       SERVO_EYES

// -----------------------------------------------------------------------------
// ANIMATOR TASK SETTINGS
// -----------------------------------------------------------------------------
#define TEACHER_SERVO_UPDATE_MS      40
#define TEACHER_SERVO_TASK_STACK     4096
#define TEACHER_SERVO_TASK_PRIORITY  5

// Eyes look path while speaking (smooth): 90 ↔ 60 ↔ 120
#define TEACHER_EYE_LOOK_LEFT_DEG    60
#define TEACHER_EYE_LOOK_CENTER_DEG  90
#define TEACHER_EYE_LOOK_RIGHT_DEG   120

// -----------------------------------------------------------------------------
// ONE ROW = ONE SERVO
// -----------------------------------------------------------------------------
// speak_mode:
//   0 = lift from home (home + amp * 0..1)  — mouth / arms
//   1 = scan around home (home + amp * sin) — eyes: 90±50 → 40..140
struct TeacherServoConfig {
    const char* name;
    gpio_num_t pin;
    ledc_channel_t ledc_ch;
    ledc_mode_t speed_mode;
    int home_deg;
    int min_deg;
    int max_deg;
    float speak_amp_deg;    // mouth/arms: signed lift; eyes: half-range (±amp around home)
    float speak_freq_hz;
    float speak_phase_rad;
    int speak_mode;         // 0 = lift, 1 = scan (sine around home)
};

// =============================================================================
// SERVO TABLE
// =============================================================================
// Amp = max travel from home while speaking (Hz/phase mostly unused; animator is gesture-based)
static const TeacherServoConfig kTeacherServos[TEACHER_SERVO_COUNT] = {
    // name           pin           ledc_ch          speed_mode              home min max   amp     Hz     phase     mode
    { "mouth",        GPIO_NUM_13,  LEDC_CHANNEL_1,  LEDC_LOW_SPEED_MODE,    15,   0,  15, -15.0f,  0.0f,  0.0f,     0 },  // home 15°, open toward 0°
    { "eyes",         GPIO_NUM_21,  LEDC_CHANNEL_2,  LEDC_LOW_SPEED_MODE,    90,  60, 120,  30.0f,  0.0f,  0.0f,     1 },  // glance 60 / 90 / 120
    { "shoulder_l",   GPIO_NUM_16,  LEDC_CHANNEL_4,  LEDC_LOW_SPEED_MODE,     0,   0,  90,  60.0f,  0.0f,  0.0f,     0 },  // home 0°, lift toward 90°
    { "elbow_l",      GPIO_NUM_12,  LEDC_CHANNEL_5,  LEDC_LOW_SPEED_MODE,    90,   0,  90, -28.0f,  0.0f,  0.0f,     0 },
    { "shoulder_r",   GPIO_NUM_23,  LEDC_CHANNEL_6,  LEDC_LOW_SPEED_MODE,    90,   0,  90, -60.0f,  0.0f,  0.0f,     0 },  // home 90°, lift toward 0°
    { "elbow_r",      GPIO_NUM_17,  LEDC_CHANNEL_0,  LEDC_LOW_SPEED_MODE,     0,   0,  90,  28.0f,  0.0f,  0.0f,     0 },
};

// -----------------------------------------------------------------------------
// SHORTCUT MACROS
// -----------------------------------------------------------------------------
#define SERVO_MOUTH_GPIO        kTeacherServos[SERVO_MOUTH].pin
#define SERVO_EYES_GPIO         kTeacherServos[SERVO_EYES].pin
#define SERVO_EYE_L_GPIO        SERVO_EYES_GPIO
#define SERVO_EYE_R_GPIO        SERVO_EYES_GPIO
#define SERVO_SHOULDER_L_GPIO   kTeacherServos[SERVO_SHOULDER_L].pin
#define SERVO_ELBOW_L_GPIO      kTeacherServos[SERVO_ELBOW_L].pin
#define SERVO_SHOULDER_R_GPIO   kTeacherServos[SERVO_SHOULDER_R].pin
#define SERVO_ELBOW_R_GPIO      kTeacherServos[SERVO_ELBOW_R].pin

#define SERVO_MOUTH_HOME        kTeacherServos[SERVO_MOUTH].home_deg
#define SERVO_EYES_HOME         kTeacherServos[SERVO_EYES].home_deg
#define SERVO_EYE_L_HOME        SERVO_EYES_HOME
#define SERVO_EYE_R_HOME        SERVO_EYES_HOME
#define SERVO_SHOULDER_L_HOME   kTeacherServos[SERVO_SHOULDER_L].home_deg
#define SERVO_ELBOW_L_HOME      kTeacherServos[SERVO_ELBOW_L].home_deg
#define SERVO_SHOULDER_R_HOME   kTeacherServos[SERVO_SHOULDER_R].home_deg
#define SERVO_ELBOW_R_HOME      kTeacherServos[SERVO_ELBOW_R].home_deg

// -----------------------------------------------------------------------------
// WIRING
// -----------------------------------------------------------------------------
// Active (6):
//   Mouth        → GPIO 13  (home 15°, human-like open toward 0°)
//   Eyes (both)  → GPIO 21  (home 90°, glance holds at 60 / 90 / 120)
//   L shoulder   → GPIO 16  (gesture / rest, not constant wave)
//   L elbow      → GPIO 12
//   R shoulder   → GPIO 23
//   R elbow      → GPIO 17
//   (GPIO 22 free)
// =============================================================================
