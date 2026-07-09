# WitBlox AI — local server with dashboard (Windows, no Docker)

Your laptop IP (Wi‑Fi): **192.168.0.103** — run `ipconfig` if this changes.

ESP32 and laptop must be on the **same Wi‑Fi**.

| Service | URL |
|---------|-----|
| **Dashboard (智控台)** | http://127.0.0.1:8001 |
| **Manager API** | http://127.0.0.1:8002/xiaozhi |
| **WebSocket (ESP32)** | `ws://192.168.0.103:8000/xiaozhi/v1/` |
| **OTA (ESP32)** | http://192.168.0.103:8002/xiaozhi/ota/ |

---

## What you need (install once)

| Software | Version | Download |
|----------|---------|----------|
| **Anaconda** | Python **3.10** env | https://www.anaconda.com/download |
| **JDK** | **21** | https://adoptium.net/ |
| **Maven** | latest | https://maven.apache.org/download.cgi |
| **Node.js** | LTS | https://nodejs.org/ |
| **MySQL** | 8.x | https://dev.mysql.com/downloads/installer/ |
| **Redis** | Windows port or Memurai | https://github.com/tporadowski/redis/releases or https://www.memurai.com/ |

**RAM:** 8 GB+ recommended for dashboard + local speech model.

**Speech model** (already downloaded if you followed earlier steps):

```
xiaozhi-esp32-server\main\xiaozhi-server\models\SenseVoiceSmall\model.pt
```

---

## Step 1 — MySQL database

1. Install MySQL, set root password to **`123456`** (or change `application-dev.yml` to match).
2. Create the database:

```sql
CREATE DATABASE xiaozhi_esp32_server CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;
```

3. Start **MySQL** service (Windows Services → MySQL).

---

## Step 2 — Redis

1. Install Redis for Windows or Memurai.
2. Start it on **port 6379** (default, no password).

---

## Step 3 — Python environment (xiaozhi-server)

Open **Anaconda Prompt** (Run as Administrator):

```powershell
conda create -n xiaozhi-esp32-server python=3.10 -y
conda activate xiaozhi-esp32-server
conda install libopus ffmpeg -y

cd C:\Users\HP\Downloads\WitBloxAiChat\xiaozhi-esp32-server\main\xiaozhi-server
pip config set global.index-url https://mirrors.aliyun.com/pypi/simple/
pip install -r requirements.txt
```

---

## Step 4 — Manager API (Java backend)

1. Set **JAVA_HOME** to JDK 21 and add **Maven** to PATH.
2. Edit DB/Redis passwords if needed:

`xiaozhi-esp32-server\main\manager-api\src\main\resources\application-dev.yml`

3. Start API (from repo root):

```powershell
cd C:\Users\HP\Downloads\WitBloxAiChat\xiaozhi-esp32-server\main\manager-api
mvn spring-boot:run
```

Wait until you see: `Started AdminApplication` and http://localhost:8002/xiaozhi/doc.html

Or run: `.\scripts\start-manager-api.ps1`

---

## Step 5 — Dashboard (manager-web)

New terminal:

```powershell
cd C:\Users\HP\Downloads\WitBloxAiChat\xiaozhi-esp32-server\main\manager-web
npm install
npm run serve
```

Open **http://127.0.0.1:8001** — register the **first user** (becomes admin).

Or run: `.\scripts\start-manager-web.ps1`

### In the dashboard (admin)

1. **参数管理** → copy **`server.secret`** value.
2. Paste into `main\xiaozhi-server\data\.config.yaml` → `manager-api.secret`.
3. **参数管理** → set **`server.websocket`** = `ws://192.168.0.103:8000/xiaozhi/v1/`
4. **参数管理** → set **`server.ota`** = `http://192.168.0.103:8002/xiaozhi/ota/`
5. **模型配置** → **大语言模型** → add your **Gemini** (or ChatGLM) API key.

---

## Step 6 — AI server (xiaozhi-server)

Anaconda Prompt:

```powershell
conda activate xiaozhi-esp32-server
cd C:\Users\HP\Downloads\WitBloxAiChat\xiaozhi-esp32-server\main\xiaozhi-server
python app.py
```

Success log includes WebSocket on port **8000**.

Or run: `.\scripts\start-xiaozhi-server.ps1`

---

## Step 7 — Windows Firewall

Allow inbound on **Private** network: **TCP 8000**, **8002**, **8001** (and **8003** if using vision).

---

## Step 8 — ESP32 firmware

OTA URL in `xiaozhi-esp32\sdkconfig.defaults.esp32` points to dashboard OTA (**port 8002**).

Rebuild and flash after IP changes:

```powershell
cd C:\Users\HP\Downloads\WitBloxAiChat\xiaozhi-esp32
idf.py build flash monitor
```

---

## Start order (every time)

1. MySQL + Redis  
2. `manager-api` (port 8002)  
3. `manager-web` (port 8001)  
4. `python app.py` (port 8000)

---

## Stop

Close each terminal window, or Ctrl+C in each service.
