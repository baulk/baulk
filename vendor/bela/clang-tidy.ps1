#!/usr/bin/env pwsh
# -- -m64 -x c++ -std=c++2a -ferror-limit=1000 -D_WIN64 -DNDEBUG -DUNICODE -D_UNICODE -D_WIN32_WINNT=0x0502 -DWINVER=0x0502 -D_CRT_SECURE_NO_WARNINGS -D_SCL_SECURE_NO_WARNINGS -DSCI_LEXER -DNO_CXX11_REGEX -I../include -Wall -Wextra -Wshadow -Wimplicit-fallthrough -Wcomma -Wformat=2


$clangtidy = Get-Command -CommandType Application "clang-tidy" -ErrorAction SilentlyContinue
if ($null -eq $clangtidy) {
    if (Test-Path "C:\Program Files (x86)\llvm\bin\clang-tidy.exe") {
        $clangtidy = "C:\Program Files (x86)\llvm\bin\clang-tidy.exe"
    }
    elseif (Test-Path "C:\Program Files\llvm\bin\clang-tidy.exe") {
        $clangtidy = "C:\Program Files\llvm\bin\clang-tidy.exe"
    }
    elseif (Test-Path "D:\LLVM\bin\clang-tidy.exe") {
        $clangtidy = "D:\LLVM\bin\clang-tidy.exe"
    }
}

$checks="-*,clang-analyzer-*,-clang-analyzer-cplusplus*,boost-*,performance-*,cert-*,readability-*,-readability-magic-numbers,modernize-*,-modernize-avoid-c-arrays,-header-filter=.*"

Write-Host "Use $clangtidy"
$clangbin=Split-Path -Parent $clangtidy
$env:PATH="$env:PATH;$clangbin"

$includes = ("*.cc", "*.c", "*.cxx", "*.cpp", "*.c++");
Get-ChildItem -Force -Recurse -File -Path "$PSScriptRoot\src" -Include $includes | ForEach-Object {
    &clang-tidy.exe $_.FullName -checks=$checks -- -m64 -x c++ -std=c++17 -ferror-limit=1000 -D_WIN64 -DNDEBUG -DUNICODE -D_UNICODE -D_WIN32_WINNT=0x0A00 -DWINVER=0x0A00 -I"$PSScriptRoot/include" -Wall -Wextra -Wshadow -Wimplicit-fallthrough
}