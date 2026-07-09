# Start web dashboard (智控台 UI). Requires manager-api on port 8002.
# Usage: .\scripts\start-manager-web.ps1

$webRoot = Join-Path $PSScriptRoot "..\xiaozhi-esp32-server\main\manager-web"
Set-Location $webRoot
if (-not (Test-Path "node_modules")) {
    Write-Host "First run: installing npm packages..." -ForegroundColor Yellow
    npm install
}
Write-Host "Starting dashboard at http://127.0.0.1:8001 ..." -ForegroundColor Cyan
npm run serve
