@echo off
setlocal

start "Ultima 6 v1.3" /D "%~dp0" "%~dp0Exult.exe" -c "%LOCALAPPDATA%\Exult\exult.cfg" --bg --mod Ultima6v1.3

endlocal
