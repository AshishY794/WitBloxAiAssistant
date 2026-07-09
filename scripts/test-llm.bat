@echo off
REM WitBlox Gemini LLM test (works in Anaconda Prompt / CMD)
cd /d "%~dp0..\xiaozhi-esp32-server\main\xiaozhi-server"
echo Running WitBlox LLM test...
python "%~dp0test-llm.py"
if errorlevel 1 (
    echo.
    echo LLM test FAILED. Check your API key in data\.config.yaml
    exit /b 1
)
echo.
echo LLM test PASSED.
exit /b 0
