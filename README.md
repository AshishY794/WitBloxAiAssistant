# WitBlox AI

Voice assistant built on the [XiaoZhi](https://github.com/78/xiaozhi-esp32) stack, customized for WitBlox (ESP32 firmware + local server with web dashboard).

## Repository layout

| Folder | Description |
|--------|-------------|
| `xiaozhi-esp32/` | ESP32 firmware (bread-compact-esp32 board, WitBloxAi Wi‑Fi hotspot) |
| `xiaozhi-esp32-server/` | Backend: AI server, manager API, web dashboard |
| `scripts/` | PowerShell helpers to start each service |
| `LOCAL_SERVER_SETUP.md` | **Windows native setup** (no Docker) |

## Quick start

1. **Server + dashboard** — follow [LOCAL_SERVER_SETUP.md](LOCAL_SERVER_SETUP.md)
2. **Firmware** — see `xiaozhi-esp32/main/boards/bread-compact-esp32/README.md`

## WitBlox customizations

- Wi‑Fi hotspot prefix: `WitBloxAi-XXXX`
- Local OTA URL in `xiaozhi-esp32/sdkconfig.defaults.esp32` (port **8002**, dashboard mode)
- Server config: `xiaozhi-esp32-server/main/xiaozhi-server/data/.config.yaml`
- Example config: `xiaozhi-esp32-server/witblox.config.yaml.example`

## Dashboard URLs (when running)

| Service | URL |
|---------|-----|
| Dashboard | http://127.0.0.1:8001 |
| API | http://127.0.0.1:8002/xiaozhi |
| ESP32 WebSocket | `ws://<your-lan-ip>:8000/xiaozhi/v1/` |
