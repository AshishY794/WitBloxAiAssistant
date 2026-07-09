# Start manager-api (dashboard backend). Requires MySQL + Redis.
# Usage: .\scripts\start-manager-api.ps1

$apiRoot = Join-Path $PSScriptRoot "..\xiaozhi-esp32-server\main\manager-api"
Set-Location $apiRoot
Write-Host "Starting manager-api at http://127.0.0.1:8002 ..." -ForegroundColor Cyan
mvn spring-boot:run
