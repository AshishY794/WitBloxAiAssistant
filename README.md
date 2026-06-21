# WitBlox AI

Voice assistant built on the [XiaoZhi](https://github.com/78/xiaozhi-esp32) stack, customized for WitBlox (ESP32 firmware + local server).

## Repository layout

| Folder | Description |
|--------|-------------|
| `xiaozhi-esp32/` | ESP32 firmware (bread-compact-esp32 board, WitBloxAi Wi‑Fi hotspot) |
| `xiaozhi-esp32-server/` | Python backend (WebSocket, ASR, LLM, TTS) |
| `LOCAL_SERVER_SETUP.md` | Windows / Docker setup on your laptop |

## Quick start

1. **Server** — follow [LOCAL_SERVER_SETUP.md](LOCAL_SERVER_SETUP.md)
2. **Firmware** — see `xiaozhi-esp32/main/boards/bread-compact-esp32/README.md`

## WitBlox customizations

- Wi‑Fi hotspot prefix: `WitBloxAi-XXXX`
- Local OTA URL in `xiaozhi-esp32/sdkconfig.defaults.esp32`
- Server sample config: `xiaozhi-esp32-server/witblox.config.yaml.example`

Copy the example to `xiaozhi-esp32-server/main/xiaozhi-server/data/.config.yaml` and add your Gemini API key before starting the server.
