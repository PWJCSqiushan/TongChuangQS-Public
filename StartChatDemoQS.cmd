@echo off
setlocal
cd /d "%~dp0"
start "" "%~dp0runtime\TongChuangQS.exe" --data "%~dp0data" --window 1
timeout /t 1 /nobreak >nul
start "" "%~dp0runtime\TongChuangQS.exe" --data "%~dp0data" --window 2
