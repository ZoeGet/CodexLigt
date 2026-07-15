@echo off
setlocal

rem Enable both connection methods by default.
rem Wired USB serial: --serial auto --baud 115200
rem Wireless UDP:     --udp --udp-port 4210, token loaded from config.local.json after pairing
rem Remove either part if you only want one connection method.
set "MONITOR_ARGS=--serial auto --baud 115200 --udp --udp-port 4210"

start "CodexLight Bridge" powershell.exe -NoProfile -ExecutionPolicy Bypass -WindowStyle Hidden -File "%~dp0CodexLightTray.ps1" -MonitorArgs "%MONITOR_ARGS%"
