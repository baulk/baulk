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
    "$PSScriptRoot\src\bela",
    "$PSScriptRoot\src\belahash",
    "$PSScriptRoot\src\belashl",
    "$PSScriptRoot\src\belatime",
    "$PSScriptRoot\src\belawin",
    "$PSScriptRoot\src\hazel"
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
    "-std=c++20",
    "-ferror-limit=1000",
    "-D_WIN64",
    "-DNDEBUG",
    "-DUNICODE",
    "-D_UNICODE",
    "-D_WIN32_WINNT=0x0A00",
    "-DWINVER=0x0A00", 
    "-Iinclude",
    "-Wall", 
    "-Wextra",
    "-Wshadow",
    "-Wimplicit-fallthrough"
)

$inputArgsPrefix = [string]::Join(" ", $inputArgs)

Write-Host "Use $clangtidy`n$inputArgsPrefix"

$extensions = (".cc", ".cxx", ".cpp", ".c++");

foreach ($d in $SOURCE_DIRS) {
    Write-Host -ForegroundColor Magenta "check $d"
    Get-ChildItem -Force -File -Recurse -Path $d | ForEach-Object {
        if ($extensions.Contains($_.Extension)) {
            $FileName = $_.FullName
            Write-Host -ForegroundColor Magenta "check $FileName"
            $exitCode = Start-Process -FilePath $clangtidy -ArgumentList "`"$FileName`" $inputArgsPrefix" -Wait -PassThru -NoNewWindow -WorkingDirectory $PSScriptRoot
            $exitCode | Out-Null
        }
    }
}

