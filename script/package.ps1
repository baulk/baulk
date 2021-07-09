#!/usr/bin/env pwsh

$BaulkRoot = Split-Path $PSScriptRoot
$BaulkIss = Join-Path $PSScriptRoot "baulk.iss"
Set-Location "$BaulkRoot/build"
# cleanup zip files
Remove-Item -Force *.zip
cpack -G ZIP
$item = Get-Item Baulk*.zip
$obj = Get-FileHash -Algorithm SHA256 $item.FullName
$baseName = Split-Path -Leaf $item.FullName
$env:BAULK_ASSET_NAME = "$baseName"
$hashtext = $obj.Algorithm + ":" + $obj.Hash.ToLower()
$hashtext | Out-File -Encoding utf8 -FilePath "$baseName.sum"
Write-Host "$env:BAULK_ASSET_NAME`n$hashtext"
# create setup
$MSVC_ARCH = "$env:MSVC_ARCH"
switch ($MSVC_ARCH) {
    "amd64_arm64" {
        $ArchitecturesAllowed = "arm64"
        $ArchitecturesInstallIn64BitMode = "arm64"
        break
    }
    "arm64" {
        $ArchitecturesAllowed = "arm64"
        $ArchitecturesInstallIn64BitMode = "arm64"
        break
    }
    "amd64" {
        $ArchitecturesAllowed = "x64"
        $ArchitecturesInstallIn64BitMode = "x64"
        break
    }
    Default {
        $ArchitecturesAllowed = ''
        $ArchitecturesInstallIn64BitMode = ''
    }
}
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