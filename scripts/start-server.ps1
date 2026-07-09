# Start WitBlox AI voice server (simple mode, no dashboard).
# Usage: conda activate xiaozhi-esp32-server; .\scripts\start-server.ps1

$serverRoot = Join-Path $PSScriptRoot "..\xiaozhi-esp32-server\main\xiaozhi-server"
Set-Location $serverRoot

Write-Host "WitBlox AI server" -ForegroundColor Cyan
Write-Host "  WebSocket : port 8000"
Write-Host "  OTA / HTTP: port 8003"
Write-Host ""
Write-Host "Keep this window open while the ESP32 is in use." -ForegroundColor Yellow
Write-Host "Press Ctrl+C to stop." -ForegroundColor Yellow
Write-Host ""

python app.py
