@echo off
REM Interactive TTS - type text, pick voice (English/Hindi/etc.)
cd /d "%~dp0..\xiaozhi-esp32-server\main\xiaozhi-server"
echo WitBlox Interactive TTS
echo.
python "%~dp0speak-tts.py"
pause
