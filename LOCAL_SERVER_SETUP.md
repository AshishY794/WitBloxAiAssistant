# WitBlox AI — local server on your laptop (Windows)

Your laptop IP (Wi‑Fi): **192.168.0.103**

ESP32 and laptop must be on the **same Wi‑Fi network**.

---

## What was already configured

| Item | Value |
|------|--------|
| Server config | `xiaozhi-esp32-server/main/xiaozhi-server/data/.config.yaml` |
| ESP32 OTA URL | `http://192.168.0.103:8003/xiaozhi/ota/` in `sdkconfig.defaults.esp32` |
| WebSocket (device talks here) | `ws://192.168.0.103:8000/xiaozhi/v1/` |

---

## Step 1 — Install Docker Desktop (recommended on Windows)

1. Download: https://www.docker.com/products/docker-desktop/
2. Install and restart PC if asked
3. Open Docker Desktop and wait until it says **Running**

Your PC has **Python 3.13** only; the server officially needs **Python 3.10**, so **Docker is the easiest path**.

---

## Step 2 — Download speech model (~240 MB, one time)

Download **model.pt** and save to:

```
xiaozhi-esp32-server\main\xiaozhi-server\models\SenseVoiceSmall\model.pt
```

Direct link:

https://modelscope.cn/models/iic/SenseVoiceSmall/resolve/master/model.pt

Or run in PowerShell:

```powershell
cd C:\Users\HP\Downloads\WitBloxAiChat\xiaozhi-esp32-server\main\xiaozhi-server\models\SenseVoiceSmall
curl.exe -L -o model.pt "https://modelscope.cn/models/iic/SenseVoiceSmall/resolve/master/model.pt"
```

---

## Step 3 — Add free LLM API key

1. Register: https://open.bigmodel.cn/usercenter/proj-mgmt/apikeys
2. Create an API key (ChatGLM **glm-4-flash** is free tier)
3. Edit `data\.config.yaml` and replace `YOUR_ZHIPU_API_KEY` with your real key

---

## Step 4 — Start the server

```powershell
cd C:\Users\HP\Downloads\WitBloxAiChat\xiaozhi-esp32-server\main\xiaozhi-server
docker compose up -d
docker logs -f xiaozhi-esp32-server
```

Success looks like:

```text
OTA接口是     http://192.168.0.103:8003/xiaozhi/ota/
Websocket地址是 ws://192.168.0.103:8000/xiaozhi/v1/
```

**Test in browser:** open http://192.168.0.103:8003/xiaozhi/ota/  
You should see something like “OTA接口运行正常”.

---

## Step 5 — Allow Windows Firewall (if browser/device cannot connect)

Allow inbound **TCP 8000** and **8003** for Docker/Python on **Private network**.

---

## Step 6 — Flash ESP32 with local OTA URL

After changing `sdkconfig.defaults.esp32`:

```powershell
cd C:\Users\HP\Downloads\WitBloxAiChat\xiaozhi-esp32
idf.py fullclean
idf.py set-target esp32
idf.py menuconfig
# Board: Bread Compact ESP32 DevKit
idf.py -p COM3 build flash monitor
```

Device will call your laptop OTA URL instead of `api.tenclass.net`.

---

## Step 7 — Test

1. Server running on laptop  
2. ESP32 on same Wi‑Fi as laptop  
3. Watch serial monitor: should connect to `192.168.0.103`  
4. Wake word still **你好小智** (hardware model) — server persona is WitBlox in config

---

## If your laptop IP changes

1. Run `ipconfig` → note new IPv4  
2. Update `data\.config.yaml` websocket URL  
3. Update `sdkconfig.defaults.esp32` `CONFIG_OTA_URL`  
4. Rebuild and flash ESP32  

---

## Stop server

```powershell
cd C:\Users\HP\Downloads\WitBloxAiChat\xiaozhi-esp32-server\main\xiaozhi-server
docker compose down
```
