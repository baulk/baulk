#!/usr/bin/env pwsh
$BaulkRoot = Split-Path -Parent $PSScriptRoot
$BaulkTerminal = Join-Path -Path $BaulkRoot -ChildPath "baulkterminal.exe"

if (!(Test-Path $BaulkTerminal)) {
    Write-Host -ForegroundColor Red "$BaulkTerminal not exists"
    Exit 1
}

New-Item -Path 'Registry::HKEY_CLASSES_ROOT\Directory\shell\baulk' -Force | Out-Null
New-ItemProperty -Path 'Registry::HKEY_CLASSES_ROOT\Directory\shell\baulk' -Name 'MUIVerb' -PropertyType String -Value 'Baulk Terminal here' | Out-Null
New-ItemProperty -Path 'Registry::HKEY_CLASSES_ROOT\Directory\shell\baulk' -Name 'Icon' -PropertyType String -Value $BaulkTerminal | Out-Null
New-Item -Path 'Registry::HKEY_CLASSES_ROOT\Directory\shell\baulk\command' -Force | Out-Null
New-ItemProperty -Path 'Registry::HKEY_CLASSES_ROOT\Directory\shell\baulk\command' -Name '(Default)' -PropertyType String -Value "`"$BaulkTerminal`" --vs -W `"%v.`"" | Out-Null

New-Item -Path 'Registry::HKEY_CLASSES_ROOT\Directory\Background\shell\baulk' -Force | Out-Null
New-ItemProperty -Path 'Registry::HKEY_CLASSES_ROOT\Directory\Background\shell\baulk' -Name 'MUIVerb' -PropertyType String -Value 'Baulk Terminal here' | Out-Null
New-ItemProperty -Path 'Registry::HKEY_CLASSES_ROOT\Directory\Background\shell\baulk' -Name 'Icon' -PropertyType String -Value $BaulkTerminal | Out-Null
New-Item -Path 'Registry::HKEY_CLASSES_ROOT\Directory\Background\shell\baulk\command' -Force | Out-Null
New-ItemProperty -Path 'Registry::HKEY_CLASSES_ROOT\Directory\Background\shell\baulk\command' -Name '(Default)' -PropertyType String -Value "`"$BaulkTerminal`" --vs -W `"%v.`"" | Out-Null
