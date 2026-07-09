@echo off
REM Start WitBlox voice server (Anaconda Prompt / CMD)
cd /d "%~dp0..\xiaozhi-esp32-server\main\xiaozhi-server"
echo WitBlox AI server - WebSocket port 8000, OTA port 8003
echo Keep this window open. Press Ctrl+C to stop.
echo.
python app.py
