#!/usr/bin/env pwsh
# uninstall menu


Remove-Item -Path 'Registry::HKEY_CLASSES_ROOT\Directory\shell\baulkarm64' -Recurse -ErrorAction Ignore | Out-Null
Remove-Item -Path 'Registry::HKEY_CLASSES_ROOT\Directory\Background\shell\baulkarm64' -Recurse -ErrorAction Ignore | Out-Null