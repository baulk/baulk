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
    $microsoftSDKs = 'HKLM:\SOFTWARE\WOW6432Node\Microsoft\Microsoft SDKs\Windows\v10.0' 
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
$UnsrewAppxName = "Unscrew-$ArchitecturesAllowed.appx"


Write-Host "build $UnsrewAppxName "

$SourceRoot = Split-Path -Parent $PSScriptRoot
$BUILD_NUM_VERSION = "520"
if ($null -ne $env:GITHUB_RUN_NUMBER) {
    $BUILD_NUM_VERSION = "$env:GITHUB_RUN_NUMBER"
}
[string]$MainVersion = Get-Content -Encoding utf8 -Path "$SourceRoot/VERSION" -ErrorAction SilentlyContinue
$MainVersion = $MainVersion.Trim()
$FullVersion = "$MainVersion.$BUILD_NUM_VERSION"
Write-Host -ForegroundColor Yellow "make unscrew appx: $FullVersion"

$WD = Join-Path -Path $SourceRoot -ChildPath "build"
Set-Location $WD

Remove-Item -Force "$UnsrewAppxName"  -ErrorAction SilentlyContinue

$BaulkAppxPfx = Join-Path -Path $WD -ChildPath "Key.pfx"

$AppxBuildRoot = Join-Path -Path $WD -ChildPath "unscrew-appx"
Remove-Item -Force -Recurse "$AppxBuildRoot" -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Force "$AppxBuildRoot"

Copy-Item -Recurse "$WD\lib\unscrew-extension.dll" -Destination "$AppxBuildRoot"
Copy-Item -Recurse "$WD\bin\unscrew.exe" -Destination "$AppxBuildRoot"
Copy-Item -Recurse "$SourceRoot\LICENSE" -Destination "$AppxBuildRoot"

Copy-Item -Recurse "$PSScriptRoot\unscrew\Assets" -Destination "$AppxBuildRoot"

#Copy-Item "$PSScriptRoot\unscrew\AppxManifest.xml" -Destination "$AppxBuildRoot\AppxManifest.xml"
(Get-Content -Path "$PSScriptRoot\unscrew\AppxManifest.xml") -replace '@@VERSION@@', "$FullVersion" -replace '@@ARCHITECTURE@@', "$ArchitecturesAllowed" | Set-Content -Encoding utf8NoBOM "$AppxBuildRoot\AppxManifest.xml"

MakeAppx.exe pack /d "unscrew-appx\\" /p "$UnsrewAppxName" /nv

# MakeCert.exe /n "CN=528CB894-BEFF-472F-9E38-40C6DF6362E2" /r /h 0 /eku "1.3.6.1.5.5.7.3.3,1.3.6.1.4.1.311.10.3.13" /e "12/31/2099" /sv "Key.pvk" "Key.cer"
# Pvk2Pfx.exe /pvk "Key.pvk" /spc "Key.cer" /pfx "Key.pfx"

if (Test-Path $BaulkAppxPfx) {
    Write-Host -ForegroundColor Green "Use $BaulkAppxPfx sign $UnsrewAppxName"
    SignTool.exe sign /fd SHA256 /a /f "Key.pfx" "$UnsrewAppxName"
}

$Destination = Join-Path -Path $SourceRoot -ChildPath "build/destination"
Copy-Item -Force $UnsrewAppxName -Destination $Destination