@echo off
where pwsh.exe >nul 2>nul
if %ERRORLEVEL% NEQ 0 (
    echo It is recommended to install PowerShell 7^+ for a better experience.
    powershell -NoProfile -NoLogo -ExecutionPolicy unrestricted -File "%~dp0fragments.ps1" %*
) else (
    
)