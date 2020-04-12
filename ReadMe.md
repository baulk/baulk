# Baulk - Minimal Package Manager for Windows

[简体中文](./ReadMe.zh-CN.md)

## Usage

```txt
baulk - Minimal Package Manager for Windows
Usage: baulk [option] command pkg ...
  -h|--help        Show usage text and quit
  -v|--version     Show version number and quit
  -V|--verbose     Make the operation more talkative
  -F|--force       Turn on force mode. such as force update frozen package
  -P|--profile     Set profile path. default: $BaulkRoot\config\baulk.json
  -A|--user-agent  Send User-Agent <name> to server
  --https-proxy    Use this proxy. Equivalent to setting the environment variable 'HTTPS_PROXY'


Command:
  exec             Execute a command
  list             List all installed packages
  search           Search for available packages, or specific package details
  install          Install specific packages. upgrade if already installed.
  uninstall        Uninstall specific packages
  update           Update ports metadata
  upgrade          Upgrade all upgradeable packages
  freeze           Freeze specific package
  unfreeze         UnFreeze specific package
  b3sum            Calculate the BLAKE3 checksum of a file
  sha256sum        Calculate the SHA256 checksum of a file

```

![](./docs/images/baulksearch.png)


WIP

