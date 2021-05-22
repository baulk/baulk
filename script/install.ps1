#!/usr/bin/env pwsh

param(
    [String]$DestinationPath = "D:\dev"
)
if ([string]::IsNullOrEmpty($DestinationPath)) {
    Write-Host "Please set `$DestinationPath"
    exit 1
}
Write-Host "DestinationPath: $DestinationPath"

if (($PSVersionTable.PSVersion.Major) -lt 7) {
    Write-Output "PowerShell 7 or later is required to run baulk installer."
    Write-Output "Upgrade PowerShell: https://docs.microsoft.com/en-us/powershell/scripting/setup/installing-windows-powershell"
    break
}

$arch = "x86"
if ($env:PROCESSOR_ARCHITECTURE -eq "AMD64") {
    $arch = "x64" 
}
elseif ($env:PROCESSOR_ARCHITECTURE -eq "ARM64") {
    $arch = "arm64" 
}


# https://api.github.com/repos/baulk/baulk/releases/latest

$downloadURL = ""
try {
    $resp = Invoke-WebRequest -Uri "https://api.github.com/repos/baulk/baulk/releases/latest" 
    $obj = ConvertFrom-Json $resp.Content -ErrorAction SilentlyContinue
    foreach ($as in $obj.assets) {
        if ($as.browser_download_url.Contains($arch)) {
            $downloadURL = $as.browser_download_url
        }
    }
}
catch {
    Write-Host -ForegroundColor Red "query baulk api error: $_"
    exit 1
}

Write-Host "Detect baulk download url: $downloadURL"

$downloadFile = Join-Path $env:TEMP "baulk.zip"


try {
    Invoke-WebRequest -Uri $downloadURL -OutFile $downloadFile
}
catch {
    Write-Host -ForegroundColor Red "download baulk error: $_"
    exit 1
}

Function Initialize-FlatTarget {
    param(
        [String]$TopDir,
        [String]$MoveTo
    )
    $items = Get-ChildItem -Path $TopDir
    if ($items.Count -ne 1) {
        return 
    }
    if ($items[0] -isnot [System.IO.DirectoryInfo]) {
        return ;
    }
    $childdel = $items[0].FullName
    $checkdir = $childdel
    for ($i = 0; $i -lt 10; $i++) {
        $childs = Get-ChildItem $checkdir
        if ($childs.Count -eq 1 -and $childs[0] -is [System.IO.DirectoryInfo]) {
            $checkdir = $childs[0].FullName
            continue;
        }
        Move-Item -Force -Path "$checkdir/*" -Destination $MoveTo
        Remove-Item -Force -Recurse $childdel
        return 
    }
}



try {
    $BaulkDestinationPath = "$DestinationPath\baulk"
    if (Test-Path $BaulkDestinationPath) {
        Remove-Item -Force -Recurse $BaulkDestinationPath
    }
    Expand-Archive -Path $downloadFile -DestinationPath $BaulkDestinationPath
    Initialize-FlatTarget -TopDir $BaulkDestinationPath -MoveTo $BaulkDestinationPath
}
catch {
    Write-Host -ForegroundColor Red "expand baulk error: $_"
    exit 1
}

&"$DestinationPath\baulk\baulkterminal.exe"