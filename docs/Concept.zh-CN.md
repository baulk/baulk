# Baulk 概念

Baulk 是一个免安装的，极简的 Windows 包管理工具。其前身为 [clangbuilder devi](https://github.com/fstudio/clangbuilder/blob/master/bin/devi.ps1)。在搭建 LLVM 编译环境时，我们需要安装一些组件，比如 NSIS 用于创建安装包，cmake/ninja 作为构建工具，Python 处理一些生成文件，git 拉取最新源码。在迭代 clangbuilder 的过程中，逐渐的演变除了 devi 这个工具，虽然几乎没什么人使用 clangbuilder 编译 LLVM，但作为个人爱好也维护至今。devi 也增加了很多包，比如 7z，bat，riggrep，procs，ninja，nuget。devi 在安装一个软件后，会创建程序的软连接到 devi 的 `.linked` 目录，这个目录会被在 Clangbuilder UI 启动 `Windows terminal` 时加入到环境变量，这样的好处是环境变量的修改不会影响操作系统，环境变量 `PATH` 条数比较少，操作系统搜索要快一些。但有些程序还依赖其自身目录的动态连接库，这里我使用了 `launcher` 的机制，通过分析其 PE 子系统，版本信息生成 `launcher` 源码，然后编译置于 `.linked` 目录，这就解决了环境变量问题和自身依赖问题。在使用 C++17 现代化代码开发了一些工具后，于是想使用 C++17 重新实现 devi，于是便有了这个项目，当然在之前曾想过使用 Golang 编写，但 Golang 在实现 launcher 时并没有这么方便，因此还是选择使用 C++ 开发 baulk。

baulk 并不存储任何包，而是将包的版本信息元数据存储在特定的 Bucket 存储库，通过更新这些存储库获得包的更新，然后 baulk 再请求更新相应的包。

## 包


## Baulk Bucket

默认情况下，baulk 使用 [https://github.com/baulk/bucket](https://github.com/baulk/bucket) 作为源