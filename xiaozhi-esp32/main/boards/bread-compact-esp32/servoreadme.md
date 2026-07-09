# Servo Motors — Bread Compact ESP32 (WitBlox)

Guide for a **teacher-style talking robot**: when WitBlox speaks, **mouth, eyes, and hands** move together.

> **Status:** Servo support is **not implemented** in firmware yet. This document is the plan (hardware + firmware). Do not treat it as already coded.

## Goal (what you want)

| When | Behavior |
|------|----------|
| AI is **speaking** (TTS playing) | All **6 servos** animate like a teacher |
| AI is **listening / idle** | Soft idle motion or return to home pose |
| User interrupts / session ends | Stop talk loop → return to home |

### 6-servo teacher layout

| # | Part | Role while speaking |
|---|------|---------------------|
| 1 | **Mouth (jaw)** | Opens/closes in rhythm with speech (main lip-sync) |
| 2 | **Left eye** | Blink / look slightly left-right |
| 3 | **Right eye** | Blink / look slightly left-right (mirror or independent) |
| 4 | **Left hand / arm** | Small teaching gestures (wave, point, rest) |
| 5 | **Right hand / arm** | Same, alternate with left |
| 6 | **Neck / head tilt** (or eyebrow) | Nod / tilt while talking for teacher feel |

> Optional later (up to 9): second hand joint, eyebrows separate, body lean. Start with **6**.

## Quick summary

| Topic | Answer |
|-------|--------|
| Servos supported today | **0** |
| Target for teacher robot | **6** (mouth + 2 eyes + 2 hands + head) |
| Hardware room for more | Up to **9** possible later |
| Recommended wiring | Direct GPIO for 6, or **PCA9685** if you want simpler expansion |
| Power | External **5 V, 2 A+** (do not use ESP32 3.3 V) |
| Sync trigger | Device state **`kDeviceStateSpeaking`** (TTS playing) |

---

## Current board (no servos yet)

The `bread-compact-esp32` firmware today controls:

- Microphone and speaker (I2S)
- OLED display (I2C)
- Buttons, LED, and one lamp on GPIO 18

There is no `SERVO_COUNT`, servo driver, or MCP tool for servos. Other boards in this repo (Otto Robot, Electron Bot, etc.) use `SERVO_COUNT` of 4 or 6 — see their folders for reference implementations.

---

## GPIO pin map

### Already used (do not use for servos)

| GPIO | Function |
|------|----------|
| 0 | Boot button |
| 2 | Built-in LED |
| 4 | Display SDA (I2C) |
| 5 | Touch button |
| 14 | Speaker I2S BCLK |
| 15 | Display SCL (I2C) |
| 18 | Lamp |
| 19 | ASR button |
| 25 | Mic I2S WS |
| 26 | Mic I2S SCK |
| 27 | Speaker I2S LRCK |
| 32 | Mic I2S DIN |
| 33 | Speaker I2S DOUT |

Optional (if ML307 cellular module is used): GPIO **16** (RX), **17** (TX).

### Free output-capable pins (direct servo wiring)

| GPIO | Notes |
|------|-------|
| 1, 3 | UART0 — avoid if you need USB serial debug |
| 12 | Boot strap pin — can cause boot issues if pulled wrong at power-on |
| 13 | Good candidate |
| 16, 17 | Good if ML307 is not used |
| 21, 22, 23 | Good candidates |

**Input-only pins (cannot drive servos):** 34, 35, 36, 39.

On paper, up to **9** free output pins exist (`1, 3, 12, 13, 16, 17, 21, 22, 23`), but several are awkward for production use. Plan for **~6–7 direct GPIO servos** as a practical limit.

---

## Two ways to connect 9 servos

### Option A — Direct GPIO + PWM (LEDC)

Each servo needs one GPIO with 50 Hz PWM (standard hobby servo signal).

| Pros | Cons |
|------|------|
| No extra board | Uses almost all remaining GPIO |
| Simple for 1–2 servos | GPIO 1/3/12 are problematic |
| | 9 servos is at the edge of what this board allows |

ESP32 LEDC supports up to **16 PWM channels**, so 9 channels in software is fine if pins are available.

**Suggested direct-GPIO assignment (if you must):**

| Servo # | GPIO | Note |
|---------|------|------|
| 1 | 13 | Safe |
| 2 | 21 | Safe |
| 3 | 22 | Safe |
| 4 | 23 | Safe |
| 5 | 16 | Skip if using ML307 |
| 6 | 17 | Skip if using ML307 |
| 7 | 12 | Boot strap — use with care |
| 8 | 1 | Conflicts with serial TX |
| 9 | 3 | Conflicts with serial RX |

### Option B — PCA9685 I2C servo driver (recommended for 9)

Use a **PCA9685** 16-channel PWM board. It drives up to 16 servos using only the **I2C bus**.

| Pros | Cons |
|------|------|
| Supports 9+ servos easily | Extra module (~$5) |
| Only 2 wires on I2C (shared with OLED) | Slightly more firmware work |
| Frees GPIO for other sensors | |

**Wiring (share bus with OLED):**

```
ESP32          OLED + PCA9685
GPIO 4  ────── SDA
GPIO 15 ────── SCL
3.3 V   ────── VCC (logic)
GND     ────── GND (common with servo supply GND)

PCA9685 default I2C address: 0x40
(OLED is typically 0x3C — no conflict)
```

Servo **power** (red/black) goes to external **5 V**, not the ESP32.

---

## Power supply (required for multiple servos)

**Never power servos from the ESP32 3.3 V pin.**

| Servos | Approx. peak current @ 5 V |
|--------|----------------------------|
| 1 | ~500 mA |
| 4 | ~1.5 A |
| 9 | **2–4 A** |

Use a dedicated **5 V 2 A+** adapter or battery pack.

```
5 V supply (+) ──► Servo red wires (all servos)
5 V supply (−) ──► Servo black wires + ESP32 GND + PCA9685 GND
ESP32 GPIO or PCA9685 OUT ──► Servo signal (orange/yellow)
```

Always connect **common ground** between ESP32, driver, and servo power supply.

---

## Firmware work (when you implement)

For the **6-servo teacher** on `bread-compact-esp32`:

1. **`config.h`** — `SERVO_COUNT 6` + the six pin defines (or PCA9685 channels)
2. **Servo driver** — reuse oscillator/LEDC from `otto-robot/`, or PCA9685
3. **`teacher_animator`** — FreeRTOS task driven by `kDeviceStateSpeaking`
4. **MCP (optional)** — `self.teacher.home` / wave for demos
5. **Build** — `idf.py set-target esp32`, board = Bread Compact ESP32 DevKit

Reference boards (motion code patterns, not the same face layout):

| Board | Servos | Path |
|-------|--------|------|
| Otto Robot | 6 | `main/boards/otto-robot/` |
| Electron Bot | 6 | `main/boards/electron-bot/` |
| EDA Robot Pro | 4 | `main/boards/lceda-course-examples/eda-robot-pro/` |

---

## Teacher robot plan (6 servos + speak sync)

### Phase 0 — Decide hardware (day 1)

Pick **one** path:

| Path | Best when | Pins |
|------|-----------|------|
| **A. Direct GPIO** | Exactly 6 servos, fewer parts | Use GPIO **13, 21, 22, 23, 16, 17** |
| **B. PCA9685** | Easy expand to 9 later | Share I2C with OLED (GPIO 4 + 15) |

Also buy:

- 6× standard hobby servos (SG90 OK for face/hands; metal-gear if arms are heavy)
- **5 V 2–3 A** supply for all servo motors
- Common GND wire ESP32 ↔ servo PSU ↔ (PCA9685 if used)

### Phase 1 — Map angles (mechanical)

Before writing code, mark home pose on paper:

| Servo | Home (idle teacher) | Speak range (example) |
|-------|---------------------|------------------------|
| Mouth | Closed ~80° | Open/close 70–110° |
| Left eye | Center ~90° | Blink 40–90°, glance ±15° |
| Right eye | Center ~90° | Mirror left |
| Left hand | Rest at side ~45° | Gesture +20–40° |
| Right hand | Rest at side ~135° | Gesture −20–40° |
| Head | Straight ~90° | Nod ±10–15° |

Calibrate real numbers after mounting (trim later via settings).

### Phase 2 — Firmware structure (implement later)

New pieces under `bread-compact-esp32/`:

1. **`config.h`** — `#define SERVO_COUNT 6` + pin names  
   `SERVO_MOUTH`, `SERVO_EYE_L`, `SERVO_EYE_R`, `SERVO_HAND_L`, `SERVO_HAND_R`, `SERVO_HEAD`
2. **Servo driver** — reuse LEDC/`Oscillator` pattern from `otto-robot/` (or PCA9685 driver)
3. **`teacher_animator` task** — FreeRTOS loop that runs only while speaking
4. **Hook into app state** — when `Application` goes to speaking → start animator; leave speaking → home pose
5. **Optional MCP tools** — `self.teacher.home`, `self.teacher.wave` for manual demos (not required for auto talk)

Core loop idea (pseudo):

```
on_state_change:
  if new_state == Speaking:
      start_talk_animation()   // mouth + eyes + hands + head
  else if was Speaking:
      stop_talk_animation()
      move_all_to_home()
```

While speaking, run a lightweight loop (~20–50 ms):

| Channel | Motion style |
|---------|--------------|
| Mouth | Fast open/close (simple sinusoid or random flap ~3–6 Hz) — **prioritize this** |
| Eyes | Slow blink every 2–4 s + tiny left/right |
| Hands | Slow alternating “explain” wave (not frantic) |
| Head | Slow nod on a longer period (~1–2 s) |

Do **not** block audio TTS on the main thread — animation must be its own FreeRTOS task.

### Phase 3 — Voice sync quality (upgrade path)

| Level | What | Effort |
|-------|------|--------|
| **L1 (first version)** | Start motion when TTS starts; stop when TTS ends | Easy — use `kDeviceStateSpeaking` |
| **L2** | Mouth flap amplitude follows audio energy (louder = wider) | Medium — sample speaker output / RMS |
| **L3** | Per-phoneme lip sync | Hard — skip for now |

**Start with L1.** Teacher feel comes mostly from mouth + gentle hands + blinking.

### Phase 4 — Build order (recommended)

1. Wire **1 servo only** (mouth) on a free GPIO + 5 V PSU  
2. Code: move mouth open/close on Speaking state  
3. Add eyes (blink)  
4. Add head nod  
5. Add left/right hands  
6. Tune speeds so it looks like a calm teacher (not frantic)  
7. Add MCP home/wave tools if you want voice commands like “wave your hand”

### Suggested direct-GPIO map (6 servos, Path A)

| Servo | GPIO | Signal wire |
|-------|------|-------------|
| Mouth | **13** | orange/yellow |
| Left eye | **21** | |
| Right eye | **22** | |
| Left hand | **23** | |
| Right hand | **16** | |
| Head | **17** | |

Power: all servo red → **5 V external**, black → **GND common with ESP32**.

This map is set in **`teacher_servo_config.h`** (single file for pins, angles, animation).

### Build & flash (terminal)

From PowerShell (ESP-IDF must be available):

```powershell
# Load ESP-IDF (once per terminal)
. C:\Espressif\frameworks\esp-idf-v5.5.4\export.ps1

cd C:\Users\HP\Downloads\WitBloxAiChat\xiaozhi-esp32

# Build
idf.py build

# Flash + serial log (change COM3 to your port)
idf.py -p COM3 flash monitor
```

Exit monitor: `Ctrl + ]`

After flash: talk to the device — when TTS plays, the 6 servos should animate; when speech ends they return home.

### Power for 6 servos

Expect peaks around **1.5–3 A**. Use dedicated 5 V 2–3 A. Keep **common GND**. USB alone will brown out the ESP32 when all arms move.

### Success checklist

- [ ] Speaking starts → all 6 animate within ~200 ms  
- [ ] Speaking ends → return to home within ~1 s  
- [ ] Listening / idle → no wild motion (or tiny blink only)  
- [ ] ESP32 does not reboot when hands move  
- [ ] Mouth is clearly the most active part while talking  

### What not to do first

- Don’t start with 9 servos  
- Don’t power servos from ESP32 3.3 V  
- Don’t run servo `delay()` on the audio/main task  
- Don’t make hands move as fast as the mouth (looks unnatural)

---

## Recommended hardware cart (6-servo teacher)

1. 6× SG90 (or better) servos  
2. External **5 V 2–3 A** PSU  
3. Jumper wires + breadboard or shield  
4. Optional: **PCA9685** if you want to expand to 9 later  
5. Mounting: 3D print / cardboard face + arm brackets so angles don’t bind  

---

## Troubleshooting

| Problem | Likely cause |
|---------|----------------|
| Servo jitters or resets ESP | Servo powered from 3.3 V or weak USB — use 5 V supply |
| ESP32 reboots when servo moves | Current spike — add external 5 V, common GND |
| Servo does not move | Wrong GPIO, no PWM init, or signal wire not connected |
| Boot loop after adding servo on GPIO 12 | Strapping pin pulled low/high at boot — use GPIO 13/21/22 instead |
| I2C conflict | Check OLED (0x3C) vs PCA9685 (0x40) addresses |

---

## Related docs

- [Board README](README.md) — build, flash, WiFi setup
- [WitBlox project README](../../../../README.md) — server + dashboard
- [Otto Robot README](../otto-robot/README.md) — MCP servo tool examples
