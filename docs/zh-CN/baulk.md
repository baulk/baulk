[《Baulk - 开发一个简单的包管理工具历程》](https://forcemz.net/toolset/2020/07/18/Baulk/)

## 前言

相对于其他操作系统，我更习惯使用 Windows，但一直以来 Windows 缺乏官方的包管理器，而第三方无论是 Scoop 还是 Chocolatey 都无法满足我独特的需求，我的要求很简单，所有的软件包都应该使用绿色解压模式，这包括了 MSI 安装包，另外安装软件时不应该修改系统和用户环境变量。包管理工具应该足够快，还要支持创建启动器等等。

从毕业工作以来，我开发了 [clangbuilder](https://github.com/fstudio/clangbuilder)，一个简化在 Windows 上使用 Visual Studio 构建 LLVM/Clang 的工具（含 GUI 工具），为了解决安装构建 LLVM 时所需工具依赖的问题，后来多次开坑编写各种软件，多年的失败经验与认知不断积累，于是在今年这个环球同此凉热的年份，我开始了新坑 ➡ [baulk](https://github.com/baulk/baulk)，baulk 花费了我半年的下班时间，现在基本可用，ReadMe 也写好了，应该可以给大家分享一下了。

## 安装和使用 Baulk

这个步骤只有四五步，下载 baulk 二进制包，解压 baulk 二进制包，点击 baulk-terminal，更新元数据，安装你需要的软件，🆗，你可以使用 baulk 安装的软件了。

```powershell
baulk update
# i 是 install 的别名
# baulk i cmake curl 7z ...
baulk install cmake curl 7z
cmake --version
```

baulk 升级命令可以使用如下命令：

```powershell
baulk update
baulk upgrade
```

也可以使用别名：

```powershell
# u 是特殊的别名，包含 update 和 upgrade 两个步骤
# baulk update and upgrade
baulk u
```

升级 baulk 自身可以使用帮助程序 `baulk-update`

```powershell
baulk-update
```

卸载软件也很容易：

```powershell
# r 是 uninstall 的别名
baulk uninstall 7z
```

在开发 baulk 时，我就决定只支持 Windows 10 1903 或更新的版本，好处显而易见，可以使用 Windows Terminal，利用 ANSI 转义输出颜色，体验非常不错：

![](https://s1.ax1x.com/2020/07/19/UWpROA.png)

baulk 提供了 baulk-exec 在命令行中执行 baulk-exec 可以初始化 baulk 环境，还提供了 ssh-askpass-baulk，ssh-askpass-baulk 用于 [TunnelSSH](https://github.com/balibuild/tunnelssh) 某些 ssh 无法打开标准输入时请求输入密码，截图如下：

![](https://s1.ax1x.com/2020/07/19/UW9O3D.png)

在 baulk 中我引入了 VirtualEnv 机制，这种机制能够使得用户并行安装多个软件版本，在 baulk-exec，baulk-terminal 中通过指定参数支持启动任意的 VENV，baulk 还提供了 baulk-dock 能够选择按照特定的 VENV 启动环境：

![](https://s1.ax1x.com/2020/07/19/UW9wcQ.png)

在 Baulk 元数据存储库 [bucket](https://github.com/baulk/bucket)，收录了 OpenJDK8(Java)，OpenJDK9(Java)，GraalVM8(Java)，Zulu14(Java)，Go(golang)，DMD(dlang) 等支持 VENV 的包，如果有人想把 Ruby 收录一下，在 Windows 上实现类似 rbenv 功能也不是什么难事。

## Baulk 的内幕

入门说完，可以稍稍讲一下 Baulk 的实现细节，baulk 的包管理类似于 Scoop，即将包的元数据存储在 Github 上，官方源为 [baulk/bucket](https://github.com/baulk/bucket)，baulk 还可以通过编写 `$BAULK_ROOT/config/baulk.json` 添加新的源，或者删除某个源，如果不同的源中存在相同名字的包，baulk 还能根据 bucket 的权重去选择使用哪个源，但目前为止，只有 [baulk/bucket](https://github.com/baulk/bucket) 一个源。

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

`baulk update` 命令更新 bucket，这里我们使用了 Github Atom RSS 机制，以官方源为例，通过 HTTP 请求项目的 `commits/master.atom` 获得当前的最新的 commitID，如果本地不存在或者与其不同，则说明 bucket 有更新，于是 baulk 下载对应的 bucket 的压缩包，解压完成元数据的更新。然后检测本地已安装的包是否存在更新版本，存在就输出提示。

>GET https://github.com/baulk/bucket/commits/master.atom

当人们运行 `baulk upgrade` 时，就会真正的升级已安装的包，更新元数据和升级分开这种机制类似于 `apt-get`，为了简化操作，baulk 提供了 `baulk u` 别名将 update/upgrade 合并在一起简化升级。

在 Baulk 中，安装软件大的步骤只有解压和生成启动器（创建符号链接），不存在什么执行初始化脚本，修改注册表，关联文件打开方式等等等等等。我的想法是最好做到隔离互不影响，因此，在开发 baulk 的过程中，我一直时朝这个方向去设计。在 baulk 中的 package 中，存在 `extension` 的关键字，`extension` 用于描述 package 压缩包（安装包）如何被 baulk 解压缩， `extension` 支持 `zip`, `msi`, `7z`, `exe`，`tar`，baulk 按照 `extension` 的类型执行相应的解压缩程序。扩展的解压程序如下：

|扩展|解压程序|限制|
|---|---|---|
|`exe`|-|-|
|`zip`|内置|支持 deflate/bzip2/zstd，不支持加密和 deflate64（deflate64 可以使用 `7z`）|
|`msi`|内置，基于 MSI API|此方式仅作解压，和在资源管理器点击安装不同|
|`7z`|优先级：</br>baulk7z - Baulk 发行版</br>7z - 使用 baulk install 安装的</br>7z - 环境变量中的|`tar.*` 之类格式解压不能一次完成，因此建议使用 `tar` 解压 `tar.*` 压缩包|
|`tar`|优先级：</br>baulktar - BaulkTar bsdtar 的现代重构</br>bsdtar - Baulk 构建版</br>MSYS2 tar - Git for Windows 携带的</br>Windows tar |Windows 内置的 tar 不支持 xz（基于 libarchive bsdtar），但 baulk 构建的 bsdtar 支持，解压 zip 时均不不支持 deflate64|

baulk 的哲学是不要修改系统和用户环境变量，环境变量的生效应该是和终端关联或者启动器关联，因此，在 baulk 中，我编写了 baulk-terminal 和 baulk-exec 以及 baulk-dock 程序，baulk-terminal 主要用于用户通过创建桌面快捷方式或者将 baulk-terminal 添加到桌面、文件夹右键菜单，通过用户点击快速打开初始化 Baulk 环境的 Windows Terminal（如果没有安装 Windows Terminal 则打开 Windows 控制台），而 baulk-exec 则是一个启动器，在运行 baulk-exec 时，根据输入的命令行参数 baulk-exec 初始化环境变量，然后启动相关的子命令，比如 Windows 操作系统中没有安装 cmake，而 baulk 安装了 cmake，以下命令就能够正常运行：

```powershell
# 打印 cmake 版本信息
baulk-exec cmake --version
```

为了避免环境变量中 `PATH` 条目过多，降低 `SearchPath` 搜索相关命令的命中率，baulk 使用了 `links` 机制，对于一些不依赖自身目录下的 dll 的命令，我们使用创建符号链接的方式将其软连接到 baulk 根目录的 `bin\links` 目录，如果自身依赖发行携带的 dll，或者需要处理 GetModuleFileName 且没有正确处理符号链接行为的命令，我们使用 `launchers` 机制，根据命令的类型调用 MSVC 生成特定的启动器，启动器大小 5K 左右，体积能够接受。如果用户没有安装 Visual Studio，则使用 `baulk-lnk` 实现相关逻辑（baulk-lnk 需要解析 `baulk.linkmeta.json`，效率有一点点损失）。baulk 在环境变量和启动器这块做了很多事情，需要解析 PE 可执行文件的信息，还需要获得 PE 文件的版本信息，在之前使用 PowerShell 编写的 devi 中，同样是这样做的，但 PowerShell 脚本执行不太快，baulk 相比是一个巨大的效率提升。

baulk 使用 WinHTTP 实现 HTTP 下载功能，能够很好的处理代理的情况，另外，baulk 还会解析 `HTTPS_PROXY` 环境变量，还支持 `--https-proxy` 设置代理，但是，我们建议应该优先使用支持设置 Windows 系统代理的工具。

baulk 内置了 zip 提取能力，支持使用 ZSTD 压缩算法的 ZIP 文件，这比市面上很多压缩软件要快一步。baulk 还内置了 `bela::hash` 支持 SHA2(SHA224, SHA256, SHA384, SHA512) SHA3(SHA3-224, SHA3-256, SHA3-384, SHA3-512)，以及 BLAKE3。由于安全问题不支持 MD5 和 SHA1，因此在 bucket 存储包哈希时应该选择使用这里列出的哈希算法，哈希字符串使用前缀匹配，默认即无前缀时为 SHA256。

```c++
  constexpr HashPrefix hnmaps[] = {
      {L"BLAKE3", hash_t::BLAKE3},     // BLAKE3
      {L"SHA224", hash_t::SHA224},     // SHA224
      {L"SHA256", hash_t::SHA256},     // SHA256
      {L"SHA384", hash_t::SHA384},     // SHA384
      {L"SHA512", hash_t::SHA512},     // SHA512
      {L"SHA3", hash_t::SHA3},         // SHA3 alias for SHA3-256
      {L"SHA3-224", hash_t::SHA3_224}, // SHA3-224
      {L"SHA3-256", hash_t::SHA3_256}, // SHA3-256
      {L"SHA3-384", hash_t::SHA3_384}, // SHA3-384
      {L"SHA3-512", hash_t::SHA3_512}, // SHA3-512
  };
```

### Baulk VirtualEnv 介绍

这里需要重点说的时 Baulk VirtualEnv，很多时候，开发者不得不并行安装一个软件的多个版本以适配不同的开发需求，但这些软件在处理环境变量的时候并没有做的那么好，因此有了 VirtualEnv 这样的工具，比如 rbenv 。baulk 目前能够很好的大多数编程语言开发工具的 VirtualEnv，以下截图是加载 Zulu14(Java JDK) 和 Go 的截图：

![](https://s1.ax1x.com/2020/07/19/UW1obq.png)

目前 baulk-terminal 和 baulk-exec 能够加载任意的 venv，baulk-dock 仅支持一个。

## 最后

baulk 花费了我很多的时间，我自己用还是很好用的，并且 baulk 沉淀了我这些年来在 Windows 系统上的技术积累，所以写一篇文章记录一下还是有一些必要的，如果有人对 baulk 里面的技术细节感兴趣，可以与我本人联系。