#!/usr/bin/env pwsh
param(
    [ValidateSet("win64", "win32", "arm64")]
    [string]$Target = "win64"
)

$TargetWithHost64s = @{
    "win64" = "amd64";
    "win32" = "amd64_x86";
    "arm64" = "amd64_arm64";
}

$TargetWithHost = $TargetWithHost64s[$Target]

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

Function Exec {
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


$VisualCxxBatchFiles = $(
    "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvarsall.bat",
    "C:\Program Files\Microsoft Visual Studio\2022\Preview\VC\Auxiliary\Build\vcvarsall.bat",
    "C:\Program Files (x86)\Microsoft Visual Studio\2019\Enterprise\VC\Auxiliary\Build\vcvarsall.bat"
)

$VisualCxxBatchFile = $null
foreach ($file in $VisualCxxBatchFiles) {
    if (Test-Path $file) {
        $VisualCxxBatchFile = $file
        break
    }
}
if ($null -eq $VisualCxxBatchFile) {
    Write-Host -ForegroundColor Red "visual c++ vcvarsall.bat not found"
    exit 1
}


Write-Host "call `"$VisualCxxBatchFile`" $TargetWithHost"

Invoke-BatchFile -Path $VisualCxxBatchFile -Arguments $TargetWithHost
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
$ExitCode = Exec -FilePath "cmake" -WD $WD -Arguments "-GNinja -DCMAKE_BUILD_TYPE=Release .."
if ($ExitCode -ne 0) {
    exit $ExitCode
}
$ExitCode = Exec -FilePath "ninja" -WD $WD -Arguments "all"
if ($ExitCode -ne 0) {
    exit $ExitCode
}