#!/usr/bin/env pwsh
# open pwsh env
$BaulkRoot = Split-Path -Parent $PSScriptRoot

function ShuffleEnv {
    $Keys = (
        "PATH", "LIB", "INCLUDE", "WindowsLibPath", "LIBPATH"
    )
    foreach ($k in $Keys) {
        $Value = [environment]::GetEnvironmentVariable($k)
        if ($null -ne $Value) {
            $vv = $Value.Split(";")
            [System.Text.StringBuilder]$newValue = New-Object -TypeName System.Text.StringBuilder
            $newValue.Capacity = $Value.Length
            foreach ($p in $vv) {
                if (![String]::IsNullOrEmpty($p)) {
                    if ($newValue.Length -ne 0) {
                        [void]$newValue.Append(";")
                    }
                    [void]$newValue.Append($p)
                }
            }
            [environment]::SetEnvironmentVariable($k, $newValue.ToString())
        }
    }
}

Function Test-AddPathEx {
    param(
        [String]$Path
    )
    if (Test-Path $Path) {
        $env:PATH = $Path + [System.IO.Path]::PathSeparator + $env:PATH
    }
}

Test-AddPathEx -Path "$BaulkRoot\bin"
Test-AddPathEx -Path "$BaulkRoot\bin\pkgs\.linked"

ShuffleEnv

$Host.UI.RawUI.WindowTitle = "Baulk 💘 Utility" 
