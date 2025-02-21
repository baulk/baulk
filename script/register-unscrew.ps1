#!/usr/bin/env pwsh
$BaulkRoot = Split-Path -Parent $PSScriptRoot
$UnscrewPath = Join-Path -Path $BaulkRoot -ChildPath "bin\unscrew.exe"

if (!(Test-Path $UnscrewPath)) {
    Write-Host -ForegroundColor Red "$UnscrewPath not exists"
    Exit 1
}

try {
    # New-Item not need '`'
    # New-ItemProperty need '`'
    New-Item -Path 'Registry::HKEY_CLASSES_ROOT\*\shell\unscrew\command' -Force | Out-Null
    New-ItemProperty -Path 'Registry::HKEY_CLASSES_ROOT\`*\shell\unscrew' -Name 'MUIVerb' -PropertyType String -Value 'Extract with Unscrew' | Out-Null
    New-ItemProperty -Path 'Registry::HKEY_CLASSES_ROOT\`*\shell\unscrew' -Name 'Icon' -PropertyType String -Value $UnscrewPath | Out-Null
    New-ItemProperty -Path 'Registry::HKEY_CLASSES_ROOT\`*\shell\unscrew\command' -Name '(Default)' -PropertyType String -Value "`"$UnscrewPath`" --flat `"%1`"" | Out-Null
}
catch {
    Write-host -Force Red "Register Unscrew failed: $_"
    Read-Host -Prompt "Press any key to continue"
}
