# Baulk - Windows 极简包管理器

Baulk Windows 极简包管理器，其设计理念来自于 [clangbuilder](https://github.com/fstudio/clangbuilder) 的 [`devi`](https://github.com/fstudio/clangbuilder/blob/master/bin/devi.ps1) 命令，devi 是一个基于 PowerShell 开发的建议包管理工具，支持安装，卸载升级包，并且还支持创建符号链接到特定的 `links` 的目录，而 `clangbuilder` 正是借此提供了 LLVM/Clang 的构建环境。devi 的理念是避免安装软件需要特权，修改操作系统注册表。以及避免软件安装过程中修改操作系统环境变量。通常来说，将特定软件添加到环境变量中确实能够简化使用，但随着安装软件的增加，环境变量会容易覆盖，多个版本的软件同时安装时不可避免的导致冲突，等等。从 2018 年 devi 的诞生到现在，我对软件安装的认识有了进步，而 devi 存在诸多问题需要解决，在思考良久后，我决定基于 C++17 和 [bela](https://github.com/fcharlie/bela) 实现新的包管理器，也就是 `Baulk`。得益于最近几年的知识积累，Baulk 要比 `devi` 更为强大，且更快，由于 PowerShell 启动较慢，执行较慢，devi 的执行速度完全比不上 baulk，实际上我在使用 Golang 重新实现基于 PowerShell 编写的 Golang 项目构建打包工具 [Bali](https://github.com/fcharlie/bali) (Golang 重新实现为 [Baligo](https://github.com/fcharlie/baligo)) 时，有相同的感受。


## 命令行参数

baulk 的命令行参数大致分三部分，第一部分是 `option`，用于指定或者设置一些变量；第二部分是 `command` 即 baulk 子命令，包括安装卸载，升级，更新，冻结，解除冻结等命令；第三部分则是跟随命令后的包名。当然具体命令具体分析，不能僵硬的理解。

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

### Baulk 配置文件

baulk 的配置文件默认路径为 `$ExecutableDir/../config/baulk.json`，可以通过设置参数 `--profile` 指定。

### Bucket 管理

在 bucket 的配置文件中，我们需要设置 `bucket`，bucket 用于存储 baulk 安装软件的源数据。bucket 目前只支持存储在 git 代码托管平台上，比如 Github。要使用 baulk 安装软件，至少得存在一个 `bucket`。baulk 默认 bucket 为 [https://github.com/baulk/bucket](https://github.com/baulk/bucket)。bucket 的配置如下：

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

在 `bucket` 中，我们设计了 `weights` 机制，在不同的 `bucket` 中，如果存在同名的包，且包的版本一致时，我们将对 `bucket` 的 `weights` 进行比较，权重较大的则会被安装。

添加 bucket 暂无命令行支持，只需要编辑 `baulk.json`，按照其格式添加即可。

同步 bucket 可以运行 `baulk update` 命令。这和 `apt update` 类似。baulk 同步 bucket 采用的时 RSS 同步机制，即通过请求 bucket 存储库获得最近的提交信息，比较最新的 commitId 与本地上一次记录的 commitId，不一致时则下载 git archive 解压到本地。这种机制的好处是不需要安装 git 便可以支持同步。

### 包管理


### 杂项




