#!/usr/bin/env pwsh
# Integrated into windows terminal 

# {
#     "profiles": [
#       {
#         "name": "Baulk",
#         "guid": "{70972808-9457-5826-a04a-cf51f621d544}",
#         "commandline": "C:/Dev/Baulk/bin/baulk-exec.exe --vs winsh",
#         "icon": "C:/Dev/Baulk/share/baulk/baulk.ico",
#         "startingDirectory": "%USERPROFILE%",
#         "tabTitle": "Windows Terminal \ud83d\udc96 Baulk"
#       }
#     ]
#   }


$BaulkRoot = Split-Path -Parent $PSScriptRoot
$BaulkLauncher = Join-Path -Path $BaulkRoot -ChildPath "bin\baulk-exec.exe"
$BaulkIcon = Join-Path -Path $BaulkRoot -ChildPath "share\baulk\baulk.ico"

if (!(Test-Path $BaulkLauncher)) {
    Write-Host -ForegroundColor Red "$BaulkLauncher not exists"
    Exit 1
}

$WindowsTerminalFragments = $env:LOCALAPPDATA + "\Microsoft\Windows Terminal\Fragments\Baulk"
if (!(Test-Path $WindowsTerminalFragments)) {
    New-Item -Force $WindowsTerminalFragments -ItemType Directory
}

$FileName = Join-Path $WindowsTerminalFragments -ChildPath "baulk-arm64.json"

$profileARM64 = @{
    name        = "Baulk ARM64";
    guid        = "`{df2527a1-229a-4b40-9d0b-664fb88ed230`}";
    commandline = "`"$BaulkLauncher`" --vs -Aarm64 winsh";
    icon        = "$BaulkIcon";
    tabTitle    = "Windows Terminal 💖 Baulk (arm64)"
}

$profilesTop = @{
    profiles = , $profileARM64
}

ConvertTo-Json $profilesTop -Depth 4 | Out-File -FilePath $FileName -Encoding utf8NoBOM