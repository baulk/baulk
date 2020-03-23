# Baulk 概念

Baulk 是一个免安装的，极简的 Windows 包管理工具。其前身为 [clangbuilder devi](https://github.com/fstudio/clangbuilder/blob/master/bin/devi.ps1)。在搭建 LLVM 编译环境时，我们需要安装一些组件来完成 LLVM 的构建打包，通过 git/curl 获取源码，使用 Python 处理构建文件，使用 cmake 生成构建文件，使用 Ninja 实施构建，最后 NSIS 用于创建安装包。这些软件也是不断升级的，如果按照原有的下载安装机制，我在不同的机器上搭建相关环境颇为不便。并且软件的更新升级也不好使。于是我便编写了迷你包管理工具，最初仅用于 LLVM 依赖软件的安装。随着 clangbuilder 的不断迭代，包管理工具逐渐的演变除了 `devi`，除了我个人外我不知道还有哪个人在使用 `devi`，但 `devi` 也维护至今。devi 目前 `ports` 了很多包，比如 7z，bat，riggrep，procs，ninja，nuget。devi 在安装一个软件后，会创建程序的软连接到 devi 的 `.linked` 目录，这个目录会被在 Clangbuilder UI 启动 `Windows terminal` 时加入到环境变量，这样的好处是环境变量的修改不会影响操作系统，环境变量 `PATH` 条数比较少，操作系统搜索要快一些。但有些程序还依赖其自身目录的动态连接库，这里我使用了 `launcher` 的机制，通过分析其 PE 子系统，版本信息生成 `launcher` 源码，然后编译置于 `.linked` 目录，这就解决了环境变量问题和自身依赖问题。`clangbuilder+devi` 汇集了我在 Windows 上开发的很多经验。但也存在一些不足，比如没有脱离 `clangbuilder`，源码和 `bucket` 没有分离，`launcher` 不支持自动生成和需要安装 `Visual C++`，受 PowerShell 因素影响，启动包含 Visual Studio 环境的终端较慢，等等。

在使用 C++17 现代化代码开发了一些工具后，于是想使用 C++17 重新实现 devi，于是便有了这个项目，当然在之前曾想过使用 Golang 编写，但 Golang 在实现 launcher 时并没有这么方便，因此最终还是选择使用 C++ 开发 baulk。


## Baulk Bucket

Bucket 用于存储软件的源数据，版本信息，下载地址，解压扩展，链接文件/启动器，Bucket 通常在 Github 这样的代码托管平台托管，在项目的 `bucket` 目录下，按照 `$PackageName.json` 的文件名存储相关包的元数据.

```json
{
    "description": "A simple, fast and user-friendly alternative to 'find'",
    "version": "v7.5.0",
    "url": "https://github.com/sharkdp/fd/releases/download/v7.5.0/fd-v7.5.0-x86_64-pc-windows-msvc.zip",
    "url.hash": "SHA256:fc66afc55d6c93d03d046cd2e593e4e499051c8e0148aa59368792ca0db439cf",
    "url64": "https://github.com/sharkdp/fd/releases/download/v7.5.0/fd-v7.5.0-i686-pc-windows-msvc.zip",
    "url64.hash": "7a0932383da31e36907654b5c7e70d0b34ae58e5c26355294ce47f96162d9533",
    "extension": "zip",
    "links": [
        "fd.exe"
    ]
}
```

在这个 `fd.json` 中，`description` 是包的描述。当存在 `url64` 下载链接时，AMD64 平台将使用这个 URL 下载包，而 x86/ARM64 则始终通过 `url` 下载包。`url.hash`,`url64.hash` 则是相关 URL 的包的校验值，其内容的前缀是哈希算法，这里我们支持 SHA256/BLAKE3，没有有效前缀时，哈希算法为 `BLAKE3`。

`extension` 则是解压缩扩展，我们支持 `msi`,`exe`,`zip`,`7z`。msi 我们使用 Windows Msi API 实现解压，`exe` 是 Windows 单个可执行文件，`zip` 我们使用 PowerShell `Expand-Archive ` cmdlet 操作。`7z` 则是使用安装的 `7za.exe` 执行。一些其他压缩格式压缩的文件能被 7z 解压的，可以设置 `extension`  为 `7z`。`links` 用于创建 symlink. `launchers` 用于创建启动器，二者的区别在于后者依赖程序目录的相关依赖或者文件，使用符号链接可能启动失败，所以使用启动器包裹。

默认情况下，baulk 使用 [https://github.com/baulk/bucket](https://github.com/baulk/bucket) 作为源