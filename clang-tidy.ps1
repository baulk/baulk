#!/usr/bin/env pwsh
# -- -m64 -x c++ -std=c++2a -ferror-limit=1000 -D_WIN64 -DNDEBUG -DUNICODE -D_UNICODE -D_WIN32_WINNT=0x0502 -DWINVER=0x0502 -D_CRT_SECURE_NO_WARNINGS -D_SCL_SECURE_NO_WARNINGS -DSCI_LEXER -DNO_CXX11_REGEX -I../include -Wall -Wextra -Wshadow -Wimplicit-fallthrough -Wcomma -Wformat=2

param(
    [string]$Out
)


$llvmArchTable = @{
    "win-x64"   = "x64";
    "win-arm64" = "arm64";
}

$RID = [System.Runtime.InteropServices.RuntimeInformation]::RuntimeIdentifier
# win-x64 win-arm64

$llvmArch = $llvmArchTable[$RID]

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

function Get-ClangTidy {
    $app = Get-Command -CommandType Application "clang-tidy" -ErrorAction SilentlyContinue
    if ($null -ne $app) {
        return  $app[0].Source
    }
    $clangTidy = Join-Path $env:ProgramFiles -ChildPath "llvm\bin\clang-tidy.exe"
    if (Test-Path $clangTidy) {
        return $clangTidy
    }
    # vswhere.exe -prerelease -latest -requires Microsoft.VisualStudio.Component.VC.Llvm.Clang -property installationPath
    $vswhere = Get-VSWhere
    if ($null -eq $vswhere) {
        return $null
    }
    $InstallDir = &$vswhere -prerelease -latest -requires Microsoft.VisualStudio.Component.VC.Llvm.Clang -property installationPath
    if ([string]::IsNullOrEmpty($InstallDir)) {
        return $null
    }
    $clangTidy = Join-Path $InstallDir -ChildPath "VC\Tools\Llvm\${llvmArch}\bin\clang-tidy.exe"
    return $clangTidy
}

$clangTidyExe = Get-ClangTidy
if ($null -eq $clangTidyExe) {
    Write-Host -ForegroundColor Red "clang-tidy not found"
    exit 1
}

$SOURCE_DIRS = $(
    "$PSScriptRoot\tools\baulk",
    "$PSScriptRoot\tools\baulk-exec",
    "$PSScriptRoot\tools\baulk-lnk",
    "$PSScriptRoot\tools\baulk-update",
    "$PSScriptRoot\lib\archive",
    "$PSScriptRoot\lib\archive\zip",
    "$PSScriptRoot\lib\archive\tar",
    "$PSScriptRoot\lib\mem",
    "$PSScriptRoot\lib\misc",
    "$PSScriptRoot\lib\net",
    "$PSScriptRoot\lib\vfs"
)

$checks = $(
    "-*",
    "clang-analyzer-*",
    "-clang-analyzer-cplusplus*",
    "performance-*",
    ## performance-no-int-to-ptr This rule is not reasonable. Calls like _get_osfhandle are warned, which is incredible. There is no operation for intptr to pointer, only conversion.
    ## disable it
    "-performance-no-int-to-ptr",
    "cert-*",
    "portability-*",
    "bugprone-*",
    "-bugprone-easily-swappable-parameters",
    "readability-*",
    "-readability-magic-numbers",
    "-readability-qualified-auto",
    "-readability-function-cognitive-complexity",
    "-readability-identifier-length",
    #"-readability-math-missing-parentheses",
    "modernize-*",
    "-modernize-use-trailing-return-type",
    "-modernize-avoid-c-arrays",
    "-header-filter=.*"
)

$checksText = [string]::Join(",", $checks)


$inputArgs = $(
    "-checks=$checksText",
    "--fix",
    "--",
    "-m64",
    "-x", 
    "c++" , 
    "-std=c++23",
    "-ferror-limit=1000",
    "-D_WIN64",
    "-DNDEBUG",
    "-DUNICODE",
    "-D_UNICODE",
    "-D_WIN32_WINNT=0x0A00",
    "-DWINVER=0x0A00", 
    "-Iinclude",
    "-Ivendor/bela/include",
    "-Ivendor/pugixml",
    "-Ilib/archive/brotli/include",
    "-Ilib/archive/bzip2",
    "-Ilib/archive/ced",
    "-Ilib/archive/chromium_zlib",
    "-Ilib/archive/liblzma/api",
    "-Ilib/archive/zstd",
    "-Wall", 
    "-Wextra",
    "-Wshadow",
    "-Wimplicit-fallthrough"
)

$inputArgsPrefix = [string]::Join(" ", $inputArgs)

Write-Host "Use $clangTidyExe`n$inputArgsPrefix"

$extensions = (".cc", ".cxx", ".cpp", ".c++");

foreach ($d in $SOURCE_DIRS) {
    Write-Host -ForegroundColor Magenta "check $d"
    Get-ChildItem -Force -File -Path $d | ForEach-Object {
        if ($extensions.Contains($_.Extension)) {
            $FileName = $_.FullName
            Write-Host -ForegroundColor Magenta "check $FileName"
            $exitCode = Start-Process -FilePath $clangTidyExe -ArgumentList "`"$FileName`" $inputArgsPrefix" -Wait -PassThru -NoNewWindow -WorkingDirectory $PSScriptRoot
            $exitCode | Out-Null
        }
    }
}

