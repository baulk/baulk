#!/usr/bin/env pwsh
$BaulkRoot = Split-Path -Parent $PSScriptRoot
$BaulkTerminal = Join-Path -Path $BaulkRoot -ChildPath "baulkterminal.exe"

if (!(Test-Path $BaulkTerminal)) {
    $BaulkRoot = Split-Path -Parent $BaulkRoot
    $BaulkTerminal = Join-Path -Path $BaulkRoot -ChildPath "baulkterminal.exe"
    if (!(Test-Path $BaulkTerminal)) {
        Write-Host -ForegroundColor Red "$BaulkTerminal not exists"
        Exit 1
    }
}

New-Item -Path 'Registry::HKEY_CLASSES_ROOT\Directory\shell\baulkarm64' -Force | Out-Null
New-ItemProperty -Path 'Registry::HKEY_CLASSES_ROOT\Directory\shell\baulkarm64' -Name 'MUIVerb' -PropertyType String -Value 'Baulk Terminal (ARM64) here' | Out-Null
New-ItemProperty -Path 'Registry::HKEY_CLASSES_ROOT\Directory\shell\baulkarm64' -Name 'Icon' -PropertyType String -Value $BaulkTerminal | Out-Null
New-Item -Path 'Registry::HKEY_CLASSES_ROOT\Directory\shell\baulkarm64\command' -Force | Out-Null
New-ItemProperty -Path 'Registry::HKEY_CLASSES_ROOT\Directory\shell\baulkarm64\command' -Name '(Default)' -PropertyType String -Value "`"$BaulkTerminal`" --vs -Aarm64 -W `"%v.`"" | Out-Null

New-Item -Path 'Registry::HKEY_CLASSES_ROOT\Directory\Background\shell\baulkarm64' -Force | Out-Null
New-ItemProperty -Path 'Registry::HKEY_CLASSES_ROOT\Directory\Background\shell\baulkarm64' -Name 'MUIVerb' -PropertyType String -Value 'Baulk Terminal (ARM64) here' | Out-Null
New-ItemProperty -Path 'Registry::HKEY_CLASSES_ROOT\Directory\Background\shell\baulkarm64' -Name 'Icon' -PropertyType String -Value $BaulkTerminal | Out-Null
New-Item -Path 'Registry::HKEY_CLASSES_ROOT\Directory\Background\shell\baulkarm64\command' -Force | Out-Null
New-ItemProperty -Path 'Registry::HKEY_CLASSES_ROOT\Directory\Background\shell\baulkarm64\command' -Name '(Default)' -PropertyType String -Value "`"$BaulkTerminal`" --vs -Aarm64 -W `"%v.`"" | Out-Null
