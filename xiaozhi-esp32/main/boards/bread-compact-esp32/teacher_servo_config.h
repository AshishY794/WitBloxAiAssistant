#pragma once

// =============================================================================
// TEACHER ROBOT — SINGLE SERVO CONFIG FILE (edit only this file)
// =============================================================================
//
// Servo layout (9 max — 7 active, 2 optional commented out):
//   • 1 mouth
//   • 2 eyes (left + right)
//   • 4 arms — shoulder + elbow per arm
//   • 2 optional (commented): head, left wrist — GPIO 1 and 3 only (free pins)
//
// GPIO budget on bread-compact-esp32 (free pins only, no PCA9685):
//   Board (always used): 0,2,4,5,14,15,18,19,25,26,27,32,33
//   7 active servos:     12,13,16,17,21,22,23
//   2 optional servos:   1, 3  (only free GPIO left)
//
// To enable optional servos:
//   1. Set TEACHER_SERVO_COUNT to 9
//   2. Uncomment SERVO_HEAD and SERVO_WRIST_L below
//   3. Uncomment the 2 rows in kTeacherServos[] and shortcut macros at bottom
//
// When WitBlox AI speaks (TTS), servos animate. When speech stops → home pose.
//
// Hardware wiring (each servo has 3 wires):
//   - Orange/Yellow → GPIO pin listed below (signal)
//   - Red           → external 5V power supply (NOT ESP32 3.3V) — use 2A+ for 9 servos
//   - Brown/Black   → GND (must connect to ESP32 GND too — common ground)
//
// After changing pins or angles: rebuild and flash firmware.
//
// =============================================================================

#include <driver/gpio.h>   // gpio_num_t, GPIO_NUM_xx
#include <driver/ledc.h>   // LEDC PWM channels for servo signals

// -----------------------------------------------------------------------------
// HOW MANY SERVOS
// -----------------------------------------------------------------------------
#define TEACHER_SERVO_COUNT 7   // Change to 9 when enabling optional servos below

#define SERVO_COUNT TEACHER_SERVO_COUNT  // old name kept for other code

// Index numbers for kTeacherServos[] — order must match table below
#define SERVO_MOUTH       0  // jaw open/close while talking
#define SERVO_EYE_L       1  // left eye
#define SERVO_EYE_R       2  // right eye
#define SERVO_SHOULDER_L  3  // left arm — shoulder (raise / wave)
#define SERVO_ELBOW_L     4  // left arm — elbow (bend)
#define SERVO_SHOULDER_R  5  // right arm — shoulder
#define SERVO_ELBOW_R     6  // right arm — elbow

// Optional servos — uncomment when enabling (also set TEACHER_SERVO_COUNT to 9)
// #define SERVO_HEAD        7  // head nod — GPIO 1 (free)
// #define SERVO_WRIST_L     8  // left wrist — GPIO 3 (free)

// -----------------------------------------------------------------------------
// ANIMATOR TASK SETTINGS (FreeRTOS background loop in teacher_animator.cc)
// -----------------------------------------------------------------------------
#define TEACHER_SERVO_UPDATE_MS      40    // How often to update servo angles (ms). 40 = ~25 times/sec
#define TEACHER_SERVO_TASK_STACK     4096  // Stack memory for servo task (bytes)
#define TEACHER_SERVO_TASK_PRIORITY  5      // Task priority (higher = runs sooner; 5 is moderate)

// -----------------------------------------------------------------------------
// EYE BLINK (only used when blink_on_speak = true for eye servos)
// -----------------------------------------------------------------------------
#define TEACHER_EYE_BLINK_INTERVAL_MS  3000  // Blink every 3 seconds while speaking
#define TEACHER_EYE_BLINK_HOLD_MS      150   // Eyes stay closed for 150 ms
#define TEACHER_EYE_BLINK_CLOSED_DEG   50    // Angle when eyes are "closed" during blink

// -----------------------------------------------------------------------------
// ONE ROW = ONE SERVO (all settings in one place)
// -----------------------------------------------------------------------------
struct TeacherServoConfig {
    const char* name;           // Name for serial log (e.g. "mouth")
    gpio_num_t pin;             // ESP32 GPIO connected to servo signal wire
    ledc_channel_t ledc_ch;     // PWM channel 0–7 (unique per speed_mode)
    ledc_mode_t speed_mode;     // LEDC_LOW_SPEED_MODE (ch 0–7) or LEDC_HIGH_SPEED_MODE (ch 0–3)
    int home_deg;               // Rest angle when AI is NOT speaking (0–180 degrees)
    int min_deg;                // Never move below this (protect mechanism)
    int max_deg;                // Never move above this (protect mechanism)
    float speak_amp_deg;        // How far to move from home while speaking (0 = no motion)
    float speak_freq_hz;        // How fast it waves while speaking (Hz)
    float speak_phase_rad;      // Phase offset in radians. Use 3.14159 (M_PI) on right side for opposite motion
    bool blink_on_speak;        // true = eyes blink periodically while speaking (ignores sine wave during blink)
};

// =============================================================================
// SERVO TABLE — change pins and angles HERE ONLY
// =============================================================================
//
// Column guide:
//   name        - label in logs
//   pin         - GPIO number on ESP32 DevKit (check config.h for mic/speaker/OLED conflicts)
//   ledc_ch     - internal PWM channel (do not duplicate within same speed_mode)
//   speed_mode  - LOW_SPEED for ch 0–7, HIGH_SPEED for extra ch 0–3
//   home        - idle pose in degrees
//   min/max     - clamp limits so servo does not hit mechanical stops
//   amp         - movement size while AI talks
//   Hz          - speed of oscillation while talking
//   phase       - 0 for most; 3.14159 on right side for opposite motion
//   blink       - true only for eye servos
//
static const TeacherServoConfig kTeacherServos[TEACHER_SERVO_COUNT] = {
    // name           pin           ledc_ch          speed_mode              home min max  amp    Hz     phase     blink
    { "mouth",        GPIO_NUM_13,  LEDC_CHANNEL_1,  LEDC_LOW_SPEED_MODE,    80,  50, 120, 20.0f, 4.5f,  0.0f,     false },  // jaw
    { "eye_l",        GPIO_NUM_21,  LEDC_CHANNEL_2,  LEDC_LOW_SPEED_MODE,    90,  40, 120, 12.0f, 0.35f, 0.0f,     true  },  // left eye + blink
    { "eye_r",        GPIO_NUM_22,  LEDC_CHANNEL_3,  LEDC_LOW_SPEED_MODE,    90,  40, 120, 12.0f, 0.35f, 0.4f,     true  },  // right eye + blink
    { "shoulder_l",   GPIO_NUM_23,  LEDC_CHANNEL_4,  LEDC_LOW_SPEED_MODE,     0,   0,  70, 15.0f, 0.6f,  0.0f,     false },  // left arm down @0°, waves up when speaking
    { "elbow_l",      GPIO_NUM_17,  LEDC_CHANNEL_5,  LEDC_LOW_SPEED_MODE,     0,   0,  90, 12.0f, 0.9f,  1.5708f,  false },  // left elbow — bends with shoulder
    { "shoulder_r",   GPIO_NUM_16,  LEDC_CHANNEL_6,  LEDC_LOW_SPEED_MODE,   135, 90, 160, 25.0f, 0.6f,  3.14159f, false },  // right shoulder — opposite wave
    { "elbow_r",      GPIO_NUM_12,  LEDC_CHANNEL_0,  LEDC_LOW_SPEED_MODE,    90,  60, 120, 18.0f, 0.9f,  4.71239f, false },  // right elbow — opposite bend

    // ----- OPTIONAL (free GPIO only — uncomment both + set TEACHER_SERVO_COUNT to 9) -----
    // { "head",         GPIO_NUM_1,   LEDC_CHANNEL_7,  LEDC_LOW_SPEED_MODE,    90,  70, 110, 10.0f, 0.4f,  0.0f,     false },  // free GPIO 1
    // { "wrist_l",      GPIO_NUM_3,   LEDC_CHANNEL_0,  LEDC_HIGH_SPEED_MODE,   90,  60, 120, 15.0f, 0.7f,  0.0f,     false },  // free GPIO 3
};

// -----------------------------------------------------------------------------
// OPTIONAL SHORTCUT MACROS (read values from table — do not edit here)
// -----------------------------------------------------------------------------
#define SERVO_MOUTH_GPIO        kTeacherServos[SERVO_MOUTH].pin
#define SERVO_EYE_L_GPIO        kTeacherServos[SERVO_EYE_L].pin
#define SERVO_EYE_R_GPIO        kTeacherServos[SERVO_EYE_R].pin
#define SERVO_SHOULDER_L_GPIO   kTeacherServos[SERVO_SHOULDER_L].pin
#define SERVO_ELBOW_L_GPIO      kTeacherServos[SERVO_ELBOW_L].pin
#define SERVO_SHOULDER_R_GPIO   kTeacherServos[SERVO_SHOULDER_R].pin
#define SERVO_ELBOW_R_GPIO      kTeacherServos[SERVO_ELBOW_R].pin

#define SERVO_MOUTH_HOME        kTeacherServos[SERVO_MOUTH].home_deg
#define SERVO_EYE_L_HOME        kTeacherServos[SERVO_EYE_L].home_deg
#define SERVO_EYE_R_HOME        kTeacherServos[SERVO_EYE_R].home_deg
#define SERVO_SHOULDER_L_HOME   kTeacherServos[SERVO_SHOULDER_L].home_deg
#define SERVO_ELBOW_L_HOME      kTeacherServos[SERVO_ELBOW_L].home_deg
#define SERVO_SHOULDER_R_HOME   kTeacherServos[SERVO_SHOULDER_R].home_deg
#define SERVO_ELBOW_R_HOME      kTeacherServos[SERVO_ELBOW_R].home_deg

// Uncomment when optional servos are enabled:
// #define SERVO_HEAD_GPIO         kTeacherServos[SERVO_HEAD].pin
// #define SERVO_WRIST_L_GPIO      kTeacherServos[SERVO_WRIST_L].pin
//
// #define SERVO_HEAD_HOME         kTeacherServos[SERVO_HEAD].home_deg
// #define SERVO_WRIST_L_HOME      kTeacherServos[SERVO_WRIST_L].home_deg

// -----------------------------------------------------------------------------
// WIRING QUICK REFERENCE
// -----------------------------------------------------------------------------
// Active (7):
//   Mouth        → GPIO 13
//   Left eye     → GPIO 21
//   Right eye    → GPIO 22
//   L shoulder   → GPIO 23  (home 0° = arm down, waves up to ~70° when speaking)
//   L elbow      → GPIO 17  (home 0°, bends with shoulder)
//   R shoulder   → GPIO 16
//   R elbow      → GPIO 12
//
// Optional (2 — free GPIO only):
//   Head         → GPIO 1
//   L wrist      → GPIO 3
//
// TUNING TIPS
// 1. Servo moves wrong direction → change home_deg or adjust min/max
// 2. Elbow bends too much → lower elbow speak_amp_deg (e.g. 18 → 10)
// 3. Shoulder hits body → tighten shoulder min_deg / max_deg
// 4. No movement → check 5V power + common GND with ESP32
// =============================================================================
