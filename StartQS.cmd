@echo off
setlocal
cd /d "%~dp0"
start "" "%~dp0runtime\TongChuangQS.exe" --data "%~dp0data" %*
