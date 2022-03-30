#!/usr/bin/env pwsh
# remove extract 

# remove-item need '`'
Remove-Item -Path 'Registry::HKEY_CLASSES_ROOT\`*\shell\unscrew' -Recurse -ErrorAction Ignore | Out-Null