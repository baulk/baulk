#!/usr/bin/env pwsh

param(
    [ValidateSet("win64", "arm64")]
    [string]$Target = "win64",
    [string]$RefName = ""
)

# https://jrsoftware.org/ishelp/index.php?topic=setup_architecturesallowed
$ArchitecturesAlloweds = @{
    "win64" = "x64compatible";
    "arm64" = "arm64";
}
# https://jrsoftware.org/ishelp/index.php?topic=setup_architecturesinstallin64bitmode
$ArchitecturesInstallIn64BitModes = @{
    "win64" = "x64compatible";
    "arm64" = "arm64";
}

$ArchitecturesAllowed = $ArchitecturesAlloweds[$Target]
$ArchitecturesInstallIn64BitMode = $ArchitecturesInstallIn64BitModes[$Target]

$BaulkRoot = Split-Path $PSScriptRoot
$BaulkIss = Join-Path $PSScriptRoot "baulk.iss"
Set-Location "$BaulkRoot/build"
$Destination = Join-Path -Path $BaulkRoot -ChildPath "build/destination"
New-Item -ItemType Directory -Force -Path $Destination -ErrorAction Stop
# cleanup zip files
Remove-Item -Force *.zip
cpack -G ZIP
$item = Get-Item Baulk*.zip
$obj = Get-FileHash -Algorithm SHA256 $item.FullName
$baseName = Split-Path -Leaf $item.FullName
$hashtext = $obj.Algorithm + ":" + $obj.Hash.ToLower()
$hashtext | Out-File -Encoding utf8 -FilePath "$baseName.sum"
Write-Host "$baseName`n$hashtext"
Copy-Item -Force $item.FullName -Destination $Destination

Write-Host "build $BaulkSetup.exe arch: $ArchitecturesAllowed install mode: $ArchitecturesInstallIn64BitMode"
$InnoCli = Get-Command -ErrorAction SilentlyContinue -CommandType Application "iscc.exe"
if ($null -ne $InnoCli) {
    $InnoSetup = $InnoCli.Path
}
else {
    $InnoSetup = Join-Path ${env:PROGRAMFILES(X86)} -ChildPath 'Inno Setup 6\iscc.exe'
}

$BaseNameSuffix = $ArchitecturesInstallIn64BitMode
if ($BaseNameSuffix.Length -eq 0) {
    $BaseNameSuffix = "ia32"
}
if ($ArchitecturesInstallIn64BitMode -eq "x64compatible") {
    $BaseNameSuffix = "x64"
}
$BaulkSetupExe = "BaulkSetup-$BaseNameSuffix.exe"
$BaulkUserSetupExe = "BaulkUserSetup-$BaseNameSuffix.exe"

Write-Host "Build InstallTarget: admin"

&$InnoSetup "$BaulkIss" "/dArchitecturesAllowed=$ArchitecturesAllowed" "/dArchitecturesInstallIn64BitMode=$ArchitecturesInstallIn64BitMode" "/dInstallTarget=admin"
if (!(Test-Path $BaulkSetupExe)) {
    Write-Host "create baulk installer failed: $BaulkSetupExe not found"
    exit 1
}

$setupobj = Get-FileHash -Algorithm SHA256 $BaulkSetupExe
$hashtext2 = $setupobj.Algorithm + ":" + $setupobj.Hash.ToLower()
$hashtext2 | Out-File -Encoding utf8 -FilePath "$BaulkSetupExe.sum"
Write-Host "$BaulkSetupExe `n$hashtext2"
Copy-Item -Force $BaulkSetupExe -Destination $Destination

Write-Host "Build InstallTarget: user"
&$InnoSetup "$BaulkIss" "/dArchitecturesAllowed=$ArchitecturesAllowed" "/dArchitecturesInstallIn64BitMode=$ArchitecturesInstallIn64BitMode" "/dInstallTarget=user"
if (!(Test-Path $BaulkUserSetupExe)) {
    Write-Host "create baulk user installer failed: $BaulkUserSetupExe not found"
    exit 1
}

$setupobj = Get-FileHash -Algorithm SHA256 $BaulkUserSetupExe
$hashtext2 = $setupobj.Algorithm + ":" + $setupobj.Hash.ToLower()
$hashtext2 | Out-File -Encoding utf8 -FilePath "$BaulkUserSetupExe.sum"
Write-Host "$BaulkUserSetupExe `n$hashtext2"
Copy-Item -Force $BaulkUserSetupExe -Destination $Destination