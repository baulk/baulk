param(
    [ValidateSet("win64", "arm64")]
    [string]$Target = "win64"
)

$ArchitecturesAlloweds = @{
    "win64" = "x64";
    "arm64" = "arm64";
}

$ArchitecturesAllowed = $ArchitecturesAlloweds[$Target]
$BaulkAppxName = "Baulk-$ArchitecturesAllowed.appx"

Remove-Item -Force "$BaulkAppxName"
Write-Host "build $BaulkAppxName "

$SourceRoot = Split-Path -Parent $PSScriptRoot
$WD = Join-Path -Path $SourceRoot -ChildPath "build"
Set-Location $WD

$BaulkAppxPfx = Join-Path -Path $WD -ChildPath "Key.pfx"

$AppxBuildRoot = Join-Path -Path $WD -ChildPath "appx"
Remove-Item -Force -Recurse "$AppxBuildRoot"

New-Item -ItemType Directory -Force "$AppxBuildRoot\bin"
New-Item -ItemType Directory -Force "$AppxBuildRoot\config"
New-Item -ItemType Directory -Force "$AppxBuildRoot\share\baulk"
New-Item -ItemType Directory -Force "$AppxBuildRoot\script"

# bin
Copy-Item -Recurse "$WD\bin\baulk.exe" -Destination "$AppxBuildRoot\bin"
Copy-Item -Recurse "$WD\bin\baulk-dock.exe" -Destination "$AppxBuildRoot\bin"
Copy-Item -Recurse "$WD\bin\baulk-exec.exe" -Destination "$AppxBuildRoot\bin"
Copy-Item -Recurse "$WD\bin\baulk-lnk.exe" -Destination "$AppxBuildRoot\bin"
Copy-Item -Recurse "$WD\bin\baulk-winlnk.exe" -Destination "$AppxBuildRoot\bin"
Copy-Item -Recurse "$WD\bin\baulk-terminal.exe" -Destination "$AppxBuildRoot"

Copy-Item -Recurse "$SourceRoot\LICENSE" -Destination "$AppxBuildRoot\share"
Copy-Item -Recurse "$SourceRoot\res\screw-driver.ico" -Destination "$AppxBuildRoot\share\baulk"

Copy-Item -Recurse "$SourceRoot\script\Fragments.ps1" -Destination "$AppxBuildRoot\script"
Copy-Item -Recurse "$SourceRoot\script\Fragments.bat" -Destination "$AppxBuildRoot\script"
Copy-Item -Recurse "$SourceRoot\script\FragmentsARM64.ps1" -Destination "$AppxBuildRoot\script"
Copy-Item -Recurse "$SourceRoot\script\FragmentsARM64.bat" -Destination "$AppxBuildRoot\script"
Copy-Item -Recurse "$SourceRoot\script\FragmentsDel.bat" -Destination "$AppxBuildRoot\script"

Copy-Item -Recurse "$PSScriptRoot\Assets" -Destination "$AppxBuildRoot"

Copy-Item "$PSScriptRoot\AppxManifest-$Target.xml" -Destination "$AppxBuildRoot\AppxManifest.xml"

MakeAppx.exe pack /d "appx\\" /p "$BaulkAppxName" /nv

# MakeCert.exe /n "CN=528CB894-BEFF-472F-9E38-40C6DF6362E2" /r /h 0 /eku "1.3.6.1.5.5.7.3.3,1.3.6.1.4.1.311.10.3.13" /e "12/31/2099" /sv "Key.pvk" "Key.cer"
# Pvk2Pfx.exe /pvk "Key.pvk" /spc "Key.cer" /pfx "Key.pfx"

if (!(Test-Path $BaulkAppxPfx)) {
    $BAULK_SECRETS_APPX_PFX = $env:BAULK_SECRETS_APPX_PFX
    if ([string]::IsNullOrEmpty($BAULK_SECRETS_APPX_PFX)) {
        return
    }
    $ByteArray = [System.Convert]::FromBase64String($BAULK_SECRETS_APPX_PFX);
    [System.IO.File]::WriteAllBytes($BaulkAppxPfx, $ByteArray);
    Write-Host "Gen $BaulkAppxPfx success."
}

SignTool.exe sign /fd SHA256 /a /f "Key.pfx" "$BaulkAppxName"