@Echo off
pwsh -NoProfile -NoLogo -ExecutionPolicy unrestricted -File "%~dp0clang-tidy.ps1" %*
