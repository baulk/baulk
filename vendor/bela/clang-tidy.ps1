#!/usr/bin/env pwsh
# -- -m64 -x c++ -std=c++2a -ferror-limit=1000 -D_WIN64 -DNDEBUG -DUNICODE -D_UNICODE -D_WIN32_WINNT=0x0502 -DWINVER=0x0502 -D_CRT_SECURE_NO_WARNINGS -D_SCL_SECURE_NO_WARNINGS -DSCI_LEXER -DNO_CXX11_REGEX -I../include -Wall -Wextra -Wshadow -Wimplicit-fallthrough -Wcomma -Wformat=2

$VSRoot = "$env:ProgramFiles\Microsoft Visual Studio"
if (Test-Path "${env:ProgramFiles(x86)}\Microsoft Visual Studio") {
    $VSRoot = "${env:ProgramFiles(x86)}\Microsoft Visual Studio"
}


$clangtidyobj = Get-Command -CommandType Application "clang-tidy" -ErrorAction SilentlyContinue
if ($null -eq $clangtidyobj) {
    $clangtidyLocal = $(
        "$VSRoot\2019\Preview\VC\Tools\Llvm\x64\bin\clang-tidy.exe", 
        "$VSRoot\2019\Community\VC\Tools\Llvm\x64\bin\clang-tidy.exe", 
        "$VSRoot\2019\Professional\VC\Tools\Llvm\x64\bin\clang-tidy.exe",
        "$VSRoot\2019\Enterprise\VC\Tools\Llvm\x64\bin\clang-tidy.exe",
        "$VSRoot\2019\Community\VC\Tools\Llvm\bin\clang-tidy.exe", 
        "$VSRoot\2019\Professional\VC\Tools\Llvm\bin\clang-tidy.exe",
        "$VSRoot\2019\Enterprise\VC\Tools\Llvm\bin\clang-tidy.exe",
        "$env:ProgramFiles\llvm\bin\clang-tidy.exe", 
        "${env:ProgramFiles(x86)}\llvm\bin\clang-tidy.exe"
    )
    foreach ($c in $clangtidyLocal) {
        if (Test-Path $c) {
            $clangtidy = $c
            break
        }
    }
}
else {
    $clangtidy = $clangtidyobj[0].Source
}

if ($null -eq $clangtidy) {
    Write-Host -ForegroundColor Red "No clang-tidy to be found"
    return 
}

$checks = "-*,clang-analyzer-*,-clang-analyzer-cplusplus*,boost-*,performance-*,cert-*,readability-*,-readability-magic-numbers,modernize-*,-modernize-avoid-c-arrays,-header-filter=.*"

Write-Host "Use $clangtidy"

$includes = ("*.cc", "*.cxx", "*.cpp", "*.c++");
Get-ChildItem -Force -Recurse -File -Path "$PSScriptRoot\src" -Include $includes | ForEach-Object {
    &"$clangtidy" $_.FullName -checks=$checks -- -m64 -x c++ -std=c++20 -ferror-limit=1000 -D_WIN64 -DNDEBUG -DUNICODE -D_UNICODE -D_WIN32_WINNT=0x0A00 -DWINVER=0x0A00 -I"$PSScriptRoot/include" -Wall -Wextra -Wshadow -Wimplicit-fallthrough
}