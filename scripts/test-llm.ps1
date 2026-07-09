# Test Gemini LLM using data/.config.yaml (English output only).
# Usage: .\scripts\test-llm.ps1  (from Anaconda Prompt or after conda activate)

$condaHook = "C:\Users\HP\miniconda3\shell\condabin\conda-hook.ps1"
if (Test-Path $condaHook) {
    & $condaHook
    conda activate xiaozhi-esp32-server
}

$serverRoot = Join-Path $PSScriptRoot "..\xiaozhi-esp32-server\main\xiaozhi-server"
Set-Location $serverRoot

Write-Host "Running WitBlox LLM test (Gemini from .config.yaml)..." -ForegroundColor Cyan
python (Join-Path $PSScriptRoot "test-llm.py")

if ($LASTEXITCODE -ne 0) {
    Write-Host "LLM test FAILED. Check your API key in data\.config.yaml" -ForegroundColor Red
    exit $LASTEXITCODE
}

Write-Host "LLM test PASSED." -ForegroundColor Green
