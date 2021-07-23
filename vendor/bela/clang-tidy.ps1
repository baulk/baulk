#!/usr/bin/env pwsh
# -- -m64 -x c++ -std=c++2a -ferror-limit=1000 -D_WIN64 -DNDEBUG -DUNICODE -D_UNICODE -D_WIN32_WINNT=0x0502 -DWINVER=0x0502 -D_CRT_SECURE_NO_WARNINGS -D_SCL_SECURE_NO_WARNINGS -DSCI_LEXER -DNO_CXX11_REGEX -I../include -Wall -Wextra -Wshadow -Wimplicit-fallthrough -Wcomma -Wformat=2

param(
    [string]$Out
)

# C:\Program Files\Microsoft Visual Studio\2022
$VS2022Root = "$env:ProgramFiles\Microsoft Visual Studio\2022"
$VS2019Root = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\2019"

$clangtidy = $null
$clangtidyLocal = $(
    # x64
    "$VS2022Root\Preview\VC\Tools\Llvm\x64\bin\clang-tidy.exe", 
    "$VS2022Root\Community\VC\Tools\Llvm\x64\bin\clang-tidy.exe", 
    "$VS2022Root\Professional\VC\Tools\Llvm\x64\bin\clang-tidy.exe",
    "$VS2022Root\Enterprise\VC\Tools\Llvm\x64\bin\clang-tidy.exe",
    # x64
    "$VS2019Root\Community\VC\Tools\Llvm\x64\bin\clang-tidy.exe", 
    "$VS2019Root\Professional\VC\Tools\Llvm\x64\bin\clang-tidy.exe",
    "$VS2019Root\Enterprise\VC\Tools\Llvm\x64\bin\clang-tidy.exe",
    # install
    "$env:ProgramFiles\llvm\bin\clang-tidy.exe", 
    "${env:ProgramFiles(x86)}\llvm\bin\clang-tidy.exe"
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

$checks = "-*,clang-analyzer-*,-clang-analyzer-cplusplus*,boost-*,performance-*,cert-*,readability-*,-readability-magic-numbers,modernize-*,-modernize-avoid-c-arrays,-header-filter=.*"

Write-Host "Use $clangtidy`n$checks"

$includes = ("*.cc", "*.cxx", "*.cpp", "*.c++");
Get-ChildItem -Force -Recurse -File -Path "$PSScriptRoot\src" -Include $includes | ForEach-Object {
    $FileName = $_.FullName
    Write-Host -ForegroundColor Magenta "check $FileName"
    if ([string]::IsNullOrEmpty($Out)) {
        &"$clangtidy" $FileName -checks=$checks -- -m64 -x c++ -std=c++20 -ferror-limit=1000 -D_WIN64 -DNDEBUG -DUNICODE -D_UNICODE -D_WIN32_WINNT=0x0A00 -DWINVER=0x0A00 -I"$PSScriptRoot/include" -Wall -Wextra -Wshadow -Wimplicit-fallthrough 
    }
    else {

        &"$clangtidy" $FileName -checks=$checks -- -m64 -x c++ -std=c++20 -ferror-limit=1000 -D_WIN64 -DNDEBUG -DUNICODE -D_UNICODE -D_WIN32_WINNT=0x0A00 -DWINVER=0x0A00 -I"$PSScriptRoot/include" -Wall -Wextra -Wshadow -Wimplicit-fallthrough | Out-File -Append -Encoding utf8 -FilePath "$Out"
    }
}