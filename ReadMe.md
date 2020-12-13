# Baulk - Minimal Package Manager for Windows

[![license badge](https://img.shields.io/github/license/baulk/baulk.svg)](LICENSE)
[![Master Branch Status](https://github.com/baulk/baulk/workflows/CI/badge.svg)](https://github.com/baulk/baulk/actions)
[![Latest Release Downloads](https://img.shields.io/github/downloads/baulk/baulk/latest/total.svg)](https://github.com/baulk/baulk/releases/latest)
[![Total Downloads](https://img.shields.io/github/downloads/baulk/baulk/total.svg)](https://github.com/baulk/baulk/releases)
[![996.icu](https://img.shields.io/badge/link-996.icu-red.svg)](https://996.icu)

[简体中文](./ReadMe.zh-CN.md)

A minimalist Windows package manager, installation-free, without modifying system environment variables, easy to use, can be integrated with Windows Terminal, can be added to the right-click menu...

## Get Started

Download the latest version of Baulk: [https://github.com/baulk/baulk/releases/latest](https://github.com/baulk/baulk/releases/latest), then unzip it to any directory, click `baulkterminal.exe` to run and open the Windows Terminal.

![](./docs/images/getstarted.png)

```powershell
baulk update
# install some package which your need
baulk install baulktar baulk7z neovim curl wget ripgrep
# now you can run curl under Windows Terminal
curl -V
# update bucket metadata
baulk update
# upgrade all upgradeable packages
baulk upgrade
# uninstall specific packages
baulk uninstall wget
```

You can right-click to run `script/installmenu.bat` with administrator privileges to add baulkterminal to the right-click menu.

![](./docs/images/menu.png)

**This is the most basic operation. If you need to know more about baulk, you can continue to read the introduction below.**

## Baulk Usage and Details

The command line parameters of baulk are roughly divided into three parts. The first part is `option`, which is used to specify or set some variables; the second part is `command`, which is the baulk subcommand, including installation and uninstallation, upgrade, update, freeze, unfreeze, etc. Command; the third part is the package name following the command. Of course, specific orders and specific analyses cannot be rigidly understood.

```txt
baulk - Minimal Package Manager for Windows
Usage: baulk [option] command pkg ...
  -h|--help        Show usage text and quit
  -v|--version     Show version number and quit
  -V|--verbose     Make the operation more talkative
  -F|--force       Turn on force mode. such as force update frozen package
  -P|--profile     Set profile path. default: $0\config\baulk.json
  -A|--user-agent  Send User-Agent <name> to server
  --https-proxy    Use this proxy. Equivalent to setting the environment variable 'HTTPS_PROXY'


Command:
  list             List all installed packages
  search           Search for available packages, or specific package details
  install          Install specific packages. upgrade if already installed. (alias: i)
  uninstall        Uninstall specific packages. (alias: r)
  update           Update ports metadata
  upgrade          Upgrade all upgradeable packages
  freeze           Freeze specific package
  unfreeze         UnFreeze specific package
  b3sum            Calculate the BLAKE3 checksum of a file
  sha256sum        Calculate the SHA256 checksum of a file
  cleancache       Cleanup download cache

Alias:
  i  install
  r  uninstall
  u  update and upgrade

```

|Command|Description|Remarks|
|---|---|---|
|list|View installed packages|N/A|
|search|Search for packages available in Bucket|baulk search command supports file name matching mode, for example `baulk search *` will search all packages in Bucket|
|install|Install specific packages|install also has other features, when the package has been installed will rebuild the launcher, when there is a new version of the package will upgrade it, `--force` will upgrade the frozen package|
|uninstall|Uninstall a specific package|N/A|
|update|update bucket metadata|similar to Ubuntu apt update command|
|upgrade|Update packages with new versions|`--force` can upgrade frozen packages|
|freeze|Freeze specific packages|Frozen packages cannot be upgraded in regular mode|
|unfreeze|Unfreeze the package|N/A|
|b3sum|Calculate the BLAKE3 hash of the file|N/A|
|sha256sum|Calculate the SHA256 hash of the file|N/A|
|cleancache|cleanup download cache|30 days expired, all cached download file will remove when add `--force` flag|
|bucket|add, delete or list buckets|

Example:

```powershell
baulk list
baulk search *w
baulk freeze python
baulk unfreeze python
baulk update
baulk upgrade
```


### Baulk configuration file

The default path of the baulk configuration file is `$ExecutableDir/../config/baulk.json`, which can be specified by setting the parameter `--profile`. Unless you need to customize the bucket or some other operation, otherwise do not need to modify the configuration file.

### Bucket management

In the bucket configuration file, we need to set `bucket`, which is used to store the source data of the baulk installation software. Buckets currently only support storage on git code hosting platforms, such as Github. To install software using baulk, there must be at least one `bucket`. The default bucket of baulk is [https://github.com/baulk/bucket](https://github.com/baulk/bucket). The bucket configuration is as follows:

**baulk.json**:

```json
{
    "bucket": [
        {
            "description": "Baulk default bucket",
            "name": "Baulk",
            "url": "https://github.com/baulk/bucket",
            "weights": 100
        }
    ]
}
```

In `bucket`, we designed the `weights` mechanism. In different `buckets`, if there is a package with the same name and the package version is the same, we will compare the `weights` of `bucket` The larger ones will be installed.

`baulk bucket add` usage：

```txt
usage: baulk bucket add URL
       baulk bucket add BucketName URL
       baulk bucket add BucketName URL Weights
       baulk bucket add BucketName URL Weights Description
```

Delete some bucket:

```shell
baulk bucket delete BucketName
```

List buckets:

```shell
baulk bucket list
```


To synchronize buckets, you can run the `baulk update` command. This is similar to `apt update`. The baulk synchronization bucket adopts the RSS synchronization mechanism, which is to obtain the latest commit information by requesting the bucket repository, compare the latest commitId with the last commitId recorded locally, and download the git archive to decompress it locally if they are inconsistent. The advantage of this mechanism is that it can support synchronization without installing git.

### Package management

baulk uses the bucket to record the download address of the package, the file hash, and the initiator that needs to be created. The default bucket repository is [https://github.com/baulk/bucket](https://github.com/baulk/bucket), of course, you can also create a bucket according to the layout of the `baulk/bucket` repository. Baulk bucket actually draws on Scoop to a certain extent, but baulk does not force the use of file hash verification, but only supports SHA256 during verification It is different from BLAKE3 and Baulk's installation mechanism.

The commands for the baulk management package include `install`, `uninstall`, `upgrade`, `freeze` and `unfreeze`, and `list` and `search`. Installing software using baulk is very simple, the command is as follows:

```shell
# Install cmake git and 7z
baulk install cmake git 7z
```

`baulk install` will install specific packages. During the installation process, baulk will read the metadata of the specific packages from the bucket. The format of these metadata is generally as follows:

```json
{
    "description": "CMake is an open-source, cross-platform family of tools designed to build, test and package software",
    "version": "3.17.2",
    "url": [
        "https://github.com/Kitware/CMake/releases/download/v3.17.2/cmake-3.17.2-win32-x86.zip",
        "https://cmake.org/files/v3.17/cmake-3.17.2-win32-x86.zip"
    ],
    "url.hash": "SHA256:66a68a1032ad1853bcff01778ae190cd461d174d6a689e1c646e3e9886f01e0a",
    "url64": [
        "https://github.com/Kitware/CMake/releases/download/v3.17.2/cmake-3.17.2-win64-x64.zip",
        "https://cmake.org/files/v3.17/cmake-3.17.2-win64-x64.zip"
    ],
    "url64.hash": "SHA256:cf82b1eb20b6fbe583487656fcd496490ffccdfbcbba0f26e19f1c9c63b0b041",
    "extension": "zip",
    "links": [
        "bin\\cmake.exe",
        "bin\\cmake-gui.exe",
        "bin\\cmcldeps.exe",
        "bin\\cpack.exe",
        "bin\\ctest.exe"
    ]
}
```

baulk downloads the compressed package according to the URL set in the list. If there is a compressed package with the same name and the hash value matches locally, the local cache is used. baulk uses WinHTTP to download the compressed package. Currently it can better support HTTP Proxy, of course, it can also be passed Set the environment variables and command line parameters to set the proxy. baulk allows no hashes to be set in the list. The hash of baulk is set to `HashPrefix:HashContent` format. If there is no hash prefix, the default is `SHA256`. The following table is the hash algorithm supported by baulk.

|Hash algorithm|Prefix|Remarks|
|---|---|---|
|SHA224|`SHA224`||
|SHA256|`SHA256`||
|SHA384|`SHA224`||
|SHA512|`SHA224`||
|SHA3-224|`SHA3-224`||
|SHA3-256|`SHA3-256`, `SHA3`|`SHA3` prefix specific ☞ `SHA3-256`|
|SHA3-384|`SHA3-384`||
|SHA3-512|`SHA3-512`||
|BLAKE3|`BLAKE3`||

In baulk, `extension` supports `zip`, `msi`, `7z`, `exe`, `tar`, and baulk executes the corresponding decompression program according to the type of `extension`. The extended decompression procedure is as follows:

|Extended|Unzip Program|Limited|
|---|---|---|
|`exe`|-|-|
|`zip`|Built-in, based on minizip|Support deflate/bzip2/zstd, Not support encryption abd deflate64 (deflate64 can use `7z`)|
|`msi`|Built-in, based on MSI API|-|
|`7z`|Priority:</br>baulk7z-Baulk distribution</br>7z-installed using baulk install</br>7z-environment variables in the format of `tar.*` cannot be decompressed once Completed, so it is recommended to use `tar` to decompress `tar.*` compressed package|
|`tar`|Priority:</br>baulktar-modern reconstruction of BaulkTar bsdtar</br>bsdtar-Baulk build</br>MSYS2 tar-carried by Git for Windows</br>Windows tar|Windows built-in Tar does not support xz (based on libarchive bsdtar), but bsdtar built by baulk does not support deflate64 when decompressing zip|

In the manifest file, there may also be `links/launchers`, and baulk will create symbolic links for specific files according to the settings of `links`. With Visual Studio installed, baulk will create a launcher based on the `launchers` setting, if If Visual Studio is not installed, it will use `baulk-lnk` to create an analog launcher. If baulk runs on Windows x64 or ARM64 architecture, there will be some small differences, that is, the platform-related URL/Launchers/Links is preferred, as follows:

|Architecture|URL|Launchers|Links|Remarks|
|---|---|---|---|---|
|x86|url|launchers|links|-|
|x64|url64, url|launchers64, launchers|links64, links|If the launchers/links of different architectures have the same goal, you don’t need to set them separately|
|ARM64|urlarm64, url|launchersarm64, launchers|linksarm64, links|If launchers/links of different architectures have the same goal, you don’t need to set them separately|


Tips: In Windows, after starting the process, we can use `GetModuleFileNameW` to get the binary file path of the process, but when the process starts from the symbolic link, the path of the symbolic link will be used. If we only use `links` in baulk to create symbolic links to the `links` directory, there may be a problem that a specific `dll` cannot be loaded, so here we use the `launcher` mechanism to solve this problem.

When running `baulk install`, if the software has already been installed, there may be two situations. The first is that the software has not been updated, then `baulk` will rebuild `links` and `launchers`, which is applicable to different packages. In the same `links`/`launchers` installation, overwriting needs to be restored. If there is an update to the software, baulk will install the latest version of the specified package.

`baulk uninstall` will remove the package and the created launcher, symbolic link. `baulk upgrade` By searching for already installed packages, if a new version of the corresponding package exists, install the new version.

There is also a `freeze/unfreeze` command in baulk. `baulk freeze` will freeze specific packages. Using `baulk install/baulk upgrade` will skip the upgrade of these packages. However, if `baulk install/baulk upgrade` is used The `--force/-f` parameter will force the upgrade of the corresponding package. We can also use `baulk unfreeze` to unfreeze specific packages.

In baulk, we can use `baulk search pattern` to search for packages added in the bucket, where `pattern` is based on file name matching, and the rules are similar to [POSIX fnmatch](https://linux.die.net/man/3/fnmatch). Running `baulk search *` will list all packages.

In baulk, we can use `baulk list` to list all installed packages, and `baulk list pkgname` to list specific packages.

![](./docs/images/baulksearch.png)


### Miscellaneous

Baulk provides sha256sum b3sum two commands to help users calculate file hashes.

## Baulk Virtual environment mechanism

In order to install different versions of the same software at the same time, baulk implements a virtual environment mechanism. By specifying `-Exxx` in baulkterminal or baulk-exec to load a specific package environment, for example, `-Eopenjdk15` loads openjdk15, `-Eopenjdk14` can load Openjdk14, these packages need to be configured in the bucket warehouse. In addition, baulk-dock can be switched graphically. Unlike baulk-exec, baulk-exec can load multiple VENVs at the same time, while baulk-dock only supports one.

## Baulk Windows Terminal integration

Baulk also provides the `baulkterminal.exe` program, which is highly integrated with Windows Terminal and can start Windows Terminal after setting the Baulk environment variable, which solves the problem of avoiding conflicts caused by tool modification of system environment variables and anytime, anywhere Contradictions of loading related environment variables, in the compressed package distributed by Baulk, we added `script/installmenu.bat` `script/installmenu.ps1` script, you can modify the registry, add a right-click menu to open Windows Terminal anytime, anywhere.

baulkterminal usage:

```txt
baulkterminal - Baulk Terminal Launcher
Usage: baulkterminal [option] ...
  -h|--help
               Show usage text and quit
  -v|--version
               Show version number and quit
  -V|--verbose
               Make the operation more talkative
  -C|--cleanup
               Create clean environment variables to avoid interference
  -S|--shell
               The shell you want to start. allowed: pwsh, bash, cmd, wsl
  -W|--cwd
               Set the shell startup directory
  -A|--arch
               Select a specific arch, use native architecture by default
  -E|--venv
               Choose to load one/more specific package virtual environment
  --vs
               Load Visual Studio related environment variables
  --conhost
               Use conhost not Windows terminal
  --clang
               Add Visual Studio's built-in clang to the PATH environment variable

```

## Baulk executor

baulk provides the `baulk-exec` command, through which we can execute some commands with the baulk environment as the background. For example, `baulk-exec pwsh` can load the baulk environment and then start pwsh. This actually has the same effect as baulkterminal, but baulk-exec can solve scenarios where Windows Terminal cannot be used, such as in a container, when performing CI/CD.

baulk-exec usage:

```txt
baulk-exec - Baulk extend executor
Usage: baulk-exec [option] command args ...
  -h|--help            Show usage text and quit
  -v|--version         Show version number and quit
  -V|--verbose         Make the operation more talkative
  -C|--cleanup         Create clean environment variables to avoid interference
  -W|--cwd             Set the command startup directory
  -A|--arch            Select a specific arch, use native architecture by default
  -E|--venv            Choose to load a specific package virtual environment
  --vs                 Load Visual Studio related environment variables
  --clang              Add Visual Studio's built-in clang to the PATH environment variable
  --unchanged-title    Keep the terminal title unchanged

example:
  baulk-exec -V --vs TUNNEL_DEBUG=1 pwsh
```

## Baulk Dock

![](./docs/images/baulk-dock.png)

## Baulk upgrade

At present, we use the Github Release Latest mechanism to upgrade Baulk itself. When executing Github Actions, when new tags are pushed, Github Actions will automatically create a release version and upload the binary compressed package. In this process, the tag information will be compiled into the baulk program. When running `baulk-update` locally (please note that baulk update is to update the bucket and baulk-update are not the same command), it will check whether the local baulk is in the tag, If it is not built on Github Actions, the next step will not be checked unless the `--force` parameter is set. If it is a tag built on Github Actions, check whether it is consistent with Github Release Latest, inconsistently download the binary of the corresponding platform, and then Update Baulk.

## Article

[《Baulk - 开发一个简单的包管理工具历程》](https://forcemz.net/toolset/2020/07/18/Baulk/)

## Other

<div>Baulk Icons made by <a href="https://www.flaticon.com/authors/smashicons" title="Smashicons">Smashicons</a> from <a href="https://www.flaticon.com/" title="Flaticon">www.flaticon.com</a></div>
