param(
    [ValidateSet("win64", "arm64")]
    [string]$Target = "win64"
)

$ArchitecturesAlloweds = @{
    "win64" = "x64";
    "arm64" = "arm64";
}

$microsoftSDKs = 'HKLM:\SOFTWARE\WOW6432Node\Microsoft\Microsoft SDKs\Windows\v10.0'
if (!(Test-Path $microsoftSDKs)) {
    $microsoftSDKs = 'HKLM:\SOFTWARE\Microsoft\Microsoft SDKs\Windows\v10.0' 
}

$sdkObject = Get-ItemProperty -Path $microsoftSDKs -ErrorAction SilentlyContinue
if ($null -eq $sdkObject) {
    Write-Host "Read $microsoftSDKs failed"
    exit 1
}
$InstallationFolder = $sdkObject.InstallationFolder
$ProductVersion = $sdkObject.ProductVersion
$binPath = Join-Path -Path $InstallationFolder -ChildPath "bin"
[string]$SdkPath = ""
Get-ChildItem -Path $binPath | ForEach-Object {
    $Name = $_.BaseName
    if ($Name.StartsWith($ProductVersion)) {
        $SdkPath = Join-Path -Path $binPath "$Name\x64";
        Write-Host "Found $SdkPath"
    }
}
if ($SdkPath.Length -ne 0) {
    $env:PATH = "$SdkPath;$env:PATH"
}

$ArchitecturesAllowed = $ArchitecturesAlloweds[$Target]
$BaulkAppxName = "Baulk-$ArchitecturesAllowed.appx"


Write-Host "build $BaulkAppxName "

$SourceRoot = Split-Path -Parent $PSScriptRoot

$BUILD_NUM_VERSION = "520"
if ($null -ne $env:GITHUB_RUN_NUMBER) {
    $BUILD_NUM_VERSION = "$env:GITHUB_RUN_NUMBER"
}
[string]$MainVersion = Get-Content -Encoding utf8 -Path "$SourceRoot/VERSION" -ErrorAction SilentlyContinue
$MainVersion = $MainVersion.Trim()
$FullVersion = "$MainVersion.$BUILD_NUM_VERSION"
Write-Host -ForegroundColor Yellow "make baulk appx: $FullVersion"

$WD = Join-Path -Path $SourceRoot -ChildPath "build"
Set-Location $WD

Remove-Item -Force "$BaulkAppxName"  -ErrorAction SilentlyContinue

$BaulkAppxPfx = Join-Path -Path $WD -ChildPath "Key.pfx"

$AppxBuildRoot = Join-Path -Path $WD -ChildPath "appx"
Remove-Item -Force -Recurse "$AppxBuildRoot" -ErrorAction SilentlyContinue

New-Item -ItemType Directory -Force "$AppxBuildRoot\bin"
New-Item -ItemType Directory -Force "$AppxBuildRoot\config"
New-Item -ItemType Directory -Force "$AppxBuildRoot\share\baulk"
New-Item -ItemType Directory -Force "$AppxBuildRoot\script"

Copy-Item -Recurse "$WD\lib\baulk-extension.dll" -Destination "$AppxBuildRoot\bin"

# bin
Copy-Item -Recurse "$WD\bin\baulk.exe" -Destination "$AppxBuildRoot\bin"
Copy-Item -Recurse "$WD\bin\baulk-dock.exe" -Destination "$AppxBuildRoot\bin"
Copy-Item -Recurse "$WD\bin\baulk-exec.exe" -Destination "$AppxBuildRoot\bin"
Copy-Item -Recurse "$WD\bin\baulk-lnk.exe" -Destination "$AppxBuildRoot\bin"
Copy-Item -Recurse "$WD\bin\baulk-winlnk.exe" -Destination "$AppxBuildRoot\bin"
Copy-Item -Recurse "$WD\bin\baulk-terminal.exe" -Destination "$AppxBuildRoot"
Copy-Item -Recurse "$WD\bin\wind.exe" -Destination "$AppxBuildRoot\bin"

Copy-Item -Recurse "$SourceRoot\LICENSE" -Destination "$AppxBuildRoot\share"
Copy-Item -Recurse "$SourceRoot\res\screw-driver.ico" -Destination "$AppxBuildRoot\share\baulk"

Copy-Item -Recurse "$SourceRoot\script\fragments.ps1" -Destination "$AppxBuildRoot\script"
Copy-Item -Recurse "$SourceRoot\script\fragments.bat" -Destination "$AppxBuildRoot\script"
Copy-Item -Recurse "$SourceRoot\script\fragments-arm64.ps1" -Destination "$AppxBuildRoot\script"
Copy-Item -Recurse "$SourceRoot\script\fragments-arm64.bat" -Destination "$AppxBuildRoot\script"
Copy-Item -Recurse "$SourceRoot\script\fragments-delete.bat" -Destination "$AppxBuildRoot\script"

Copy-Item -Recurse "$PSScriptRoot\baulk\Assets" -Destination "$AppxBuildRoot"

(Get-Content -Path "$PSScriptRoot\baulk\AppxManifest.xml") -replace '@@VERSION@@', "$FullVersion" -replace '@@ARCHITECTURE@@', "$ArchitecturesAllowed" | Set-Content -Encoding utf8NoBOM "$AppxBuildRoot\AppxManifest.xml"

MakeAppx.exe pack /d "appx\\" /p "$BaulkAppxName" /nv

# MakeCert.exe /n "CN=528CB894-BEFF-472F-9E38-40C6DF6362E2" /r /h 0 /eku "1.3.6.1.5.5.7.3.3,1.3.6.1.4.1.311.10.3.13" /e "12/31/2099" /sv "Key.pvk" "Key.cer"
# Pvk2Pfx.exe /pvk "Key.pvk" /spc "Key.cer" /pfx "Key.pfx"

if (Test-Path $BaulkAppxPfx) {
    Write-Host -ForegroundColor Green "Use $BaulkAppxPfx sign $BaulkAppxName"
    SignTool.exe sign /fd SHA256 /a /f "Key.pfx" "$BaulkAppxName"
}



$Destination = Join-Path -Path $SourceRoot -ChildPath "build/destination"
Copy-Item -Force $BaulkAppxName -Destination $Destination