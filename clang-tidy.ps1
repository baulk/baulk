#!/usr/bin/env pwsh
# -- -m64 -x c++ -std=c++2a -ferror-limit=1000 -D_WIN64 -DNDEBUG -DUNICODE -D_UNICODE -D_WIN32_WINNT=0x0502 -DWINVER=0x0502 -D_CRT_SECURE_NO_WARNINGS -D_SCL_SECURE_NO_WARNINGS -DSCI_LEXER -DNO_CXX11_REGEX -I../include -Wall -Wextra -Wshadow -Wimplicit-fallthrough -Wcomma -Wformat=2

param(
    [string]$Out
)

# C:\Program Files\Microsoft Visual Studio\2022
$VS2022Root = "$env:ProgramFiles\Microsoft Visual Studio\2022"

$clangtidy = $null
$clangtidyLocal = $(
    # x64
    "$VS2022Root\Preview\VC\Tools\Llvm\x64\bin\clang-tidy.exe", 
    "$VS2022Root\Community\VC\Tools\Llvm\x64\bin\clang-tidy.exe", 
    "$VS2022Root\Professional\VC\Tools\Llvm\x64\bin\clang-tidy.exe",
    "$VS2022Root\Enterprise\VC\Tools\Llvm\x64\bin\clang-tidy.exe",
    
    "$env:ProgramFiles\llvm\bin\clang-tidy.exe"
)
foreach ($c in $clangtidyLocal) {
    if (Test-Path $c) {
        $clangtidy = $c
        break
    }
}

if ($null -eq $clangtidy) {
    $clangtidyobj = Get-Command -CommandType Application "clang-tidy" -ErrorAction SilentlyContinue
    if ($null -eq $clangtidyobj) {
        Write-Host -ForegroundColor Red "No clang-tidy to be found"
        return 
    }
    $clangtidy = $clangtidyobj[0].Source
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
    "boost-*",
    "performance-*",
    "cert-*",
    "readability-*",
    "-readability-magic-numbers",
    "-readability-qualified-auto",
    "modernize-*",
    "-modernize-use-trailing-return-type",
    "-modernize-avoid-c-arrays",
    "-header-filter=.*"
)

$checksText = [string]::Join(",", $checks)


$inputArgs = $(
    "-checks=$checksText",
    "--",
    "-m64",
    "-x", 
    "c++" , 
    "-std=c++20",
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

Write-Host "Use $clangtidy`n$inputArgsPrefix"

$includes = ("*.cc", "*.cxx", "*.cpp", "*.c++");

foreach ($d in $SOURCE_DIRS) {
    Get-ChildItem -Force -File -Path $d -Include $includes | ForEach-Object {
        $FileName = $_.FullName
        Write-Host -ForegroundColor Magenta "check $FileName"
        $exitCode = Start-Process -FilePath $clangtidy -ArgumentList "`"$FileName`" $inputArgsPrefix" -Wait -PassThru -NoNewWindow -WorkingDirectory $PSScriptRoot
        $exitCode | Out-Null
    }
}

