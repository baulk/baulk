#!/usr/bin/env pwsh
param(
    [ValidateSet("win64", "arm64")]
    [string]$Target = "win64"
)

function Get-VSWhere {
    $app = Get-Command -CommandType Application "vswhere" -ErrorAction SilentlyContinue
    if ($null -ne $app) {
        return  $app[0].Source
    }
    $vswhere = Join-Path ${env:ProgramFiles(x86)} -ChildPath "Microsoft Visual Studio\Installer\vswhere.exe"
    if (Test-Path $vswhere) {
        return $vswhere
    }
    return $null
}

Function Invoke-BatchFile {
    param(
        [Parameter(Mandatory = $true)]
        [string] $Path,
        [string] $Arguments
    )
    Set-StrictMode -Version Latest
    $tempFile = [IO.Path]::GetTempFileName()

    cmd.exe /c " `"$Path`" $Arguments && set > `"$tempFile`" " | Out-Host
    ## Go through the environment variables in the temp file.
    ## For each of them, set the variable in our local environment.
    Get-Content $tempFile | ForEach-Object {
        if ($_ -match "^(.*?)=(.*)$") {
            Set-Content "env:\$($matches[1])" $matches[2]
        }
    }
    Remove-Item $tempFile
}

Function Execute {
    param(
        [string]$FilePath,
        [string]$Arguments,
        [string]$WD
    )
    $ProcessInfo = New-Object System.Diagnostics.ProcessStartInfo
    $ProcessInfo.FileName = $FilePath
    if ([String]::IsNullOrEmpty($WD)) {
        $ProcessInfo.WorkingDirectory = $PWD
    }
    else {
        $ProcessInfo.WorkingDirectory = $WD
    }
    Write-Host "$FilePath $Arguments [$($ProcessInfo.WorkingDirectory)]"
    #0x00000000 WindowStyle
    $ProcessInfo.Arguments = $Arguments
    $ProcessInfo.UseShellExecute = $false ## use createprocess not shellexecute
    $Process = New-Object System.Diagnostics.Process
    $Process.StartInfo = $ProcessInfo
    try {
        if ($Process.Start() -eq $false) {
            return -1
        }
        $Process.WaitForExit()
    }
    catch {
        return 127
    }
    return $Process.ExitCode
}

# code  begin
$targetTables = @{
    "win-x64@win64"   = "amd64";
    "win-x64@arm64"   = "amd64_arm64";
    "win-arm64@arm64" = "arm64";
    "win-arm64@win64" = "arm64_amd64";
}

$RID = [System.Runtime.InteropServices.RuntimeInformation]::RuntimeIdentifier # win-x64 win-arm64
$vscomponent = "Microsoft.VisualStudio.Component.VC.Tools.x86.x64"
if ($Target -eq "arm64") {
    $vscomponent = "Microsoft.VisualStudio.Component.VC.Tools.ARM64"
}

$vsarch = $targetTables["$RID@$Target"]

$vswhere = Get-VSWhere
if ($null -eq $vswhere) {
    Write-Host -ForegroundColor Red "No vswhere installation found"
    exit 1
}

# vswhere -latest -prerelease -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$vsInstallDir = &$vswhere -latest  -products * -requires $vscomponent -property installationPath
if ([string]::IsNullOrEmpty($vsInstallDir)) {
    $vsInstallDir = &$vswhere -latest -prerelease -products * -requires $vscomponent -property installationPath
}
if ([string]::IsNullOrEmpty($vsInstallDir)) {
    Write-Host -ForegroundColor Red "No Visual Studio installation found."
    exit 1
}

$VisualCxxBatchFile = Join-Path $vsInstallDir -ChildPath "VC\Auxiliary\Build\vcvarsall.bat"

Write-Host "call `"$VisualCxxBatchFile`" $vsarch"

Invoke-BatchFile -Path $VisualCxxBatchFile -Arguments $vsarch

$cmake = Get-Command -CommandType Application "cmake" -ErrorAction SilentlyContinue
if ($null -ne $cmake) {
    $cmakeExe = $cmake[0].Source
    Write-Host "Use cmake $cmakeExe"
}

$WD = Join-Path -Path $PWD -ChildPath "build"
try {
    New-Item -ItemType Directory -Force -Path $WD
}
catch {
    Write-Host -ForegroundColor Red "mkdir error $_"
    exit 1
}
$env:CC = "cl"
$env:CXX = "cl"
$ExitCode = Execute -FilePath "cmake" -WD $WD -Arguments "-GNinja -DCMAKE_BUILD_TYPE=Release -DENABLE_BELA_TEST=ON .."
if ($ExitCode -ne 0) {
    exit $ExitCode
}
$ExitCode = Execute -FilePath "ninja" -WD $WD -Arguments "all"
if ($ExitCode -ne 0) {
    exit $ExitCode
}
