@echo off
REM WitBlox STT test - speak into mic, see text (FunASR local)
cd /d "%~dp0..\xiaozhi-esp32-server\main\xiaozhi-server"
echo WitBlox STT Test (Speech to Text)
echo First run loads the AI model - wait 30-60 seconds.
echo.
python "%~dp0test-stt.py"
pause
