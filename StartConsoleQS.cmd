@echo off
setlocal
cd /d "%~dp0"
"%~dp0runtime\SocialQS.exe" --data "%~dp0data" %*
pause
