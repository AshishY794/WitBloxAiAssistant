@echo off
REM WitBlox TTS test - speaks text, saves MP3, opens player (Anaconda Prompt / CMD)
cd /d "%~dp0..\xiaozhi-esp32-server\main\xiaozhi-server"
echo Running WitBlox TTS test...
python "%~dp0test-tts.py"
if errorlevel 1 (
    echo.
    echo TTS test FAILED. Check internet connection and data\.config.yaml
    exit /b 1
)
echo.
exit /b 0
