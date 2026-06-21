# Bread Compact ESP32 DevKit

Board profile for classic **ESP32** (not ESP32-S3). Use this when your module is **ESP32-WROOM** / **ESP32-DevKitC**, not S3/C3.

## Build & flash

**Set target to ESP32:**

```bash
idf.py set-target esp32
idf.py fullclean
idf.py menuconfig
```

**Select board in menuconfig:**

```
Xiaozhi Assistant -> Board Type -> 面包板 ESP32 DevKit (Bread Compact ESP32 DevKit)
```

**Build, erase flash, and upload** (replace `COM3` with your port):

```bash
idf.py -p COM3 build erase-flash flash monitor
```

Exit serial monitor: `Ctrl + ]`

## Important: fix WiFi reboot loop (stack overflow)

Classic ESP32 has less RAM than ESP32-S3. XiaoZhi v2 can crash during WiFi connect with:

```text
***ERROR*** A stack overflow in task sys_evt has been detected.
```

The device then reboots in a loop when connecting to saved WiFi.

**Fix — increase the system event task stack** (required for stable WiFi on ESP32):

This project sets **4096** in `sdkconfig.defaults.esp32` (project root). After changing it, run `idf.py fullclean` then rebuild.

To change manually in menuconfig:

1. Run `idf.py menuconfig`
2. Go to **Component config → ESP System Settings → Event loop task stack size**
3. Change **2304 → 4096** (use **6144** if it still crashes)
4. Save, exit, then rebuild and flash:

```bash
idf.py fullclean
idf.py -p COM3 build flash monitor
```

Default in `sdkconfig.defaults.esp32`:

```text
CONFIG_ESP_SYSTEM_EVENT_TASK_STACK_SIZE=4096
```

## menuconfig — when is it required?

| Situation | Run menuconfig? |
|-----------|-----------------|
| First build | Yes — pick **Bread Compact ESP32 DevKit** |
| After `idf.py set-target esp32` | Yes — confirm board + stack size |
| Same board, already configured | Optional |
| Wrong chip flashed (S3 vs ESP32) | Yes — fix target and board |

`set-target` chooses the **chip**. menuconfig chooses the **board** (pins, display, flash layout). Both matter.

## Chip vs target

| Module label | Correct command |
|--------------|-----------------|
| ESP32 / ESP32-WROOM | `idf.py set-target esp32` |
| ESP32-S3 | `idf.py set-target esp32s3` |
| ESP32-C3 | `idf.py set-target esp32c3` |

If esptool says `This chip is ESP32, not ESP32-S3`, your target is wrong — use `set-target esp32`.

## WiFi setup (first boot)

1. Device starts a hotspot like `WitBloxAi-XXXX`
2. Connect your phone, open the captive portal page
3. Select your router WiFi and enter the password
4. Device saves credentials and reboots

Prefer a **home router** over a phone hotspot when possible. iPhone hotspots can be less stable for ESP32.

## Optional menuconfig tweaks

- **Boya flash chip** warning: enable `Component config → SPI Flash driver → Support for Boya flash` (`SPI_FLASH_SUPPORT_BOYA_CHIP`)
- Still crashing after WiFi connect: also raise **Component config → LWIP → TCPIP task stack size** to **4096**

## Compile only

```bash
idf.py build
```
