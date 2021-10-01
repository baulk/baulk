# Baulk 4.0 变更技术细节

## Baulk VFS 布局方案

随着 Baulk 开始为用户提供安装包以及准备通过 Windows Store (Windows 11) 发布，传统的开箱即用的便携性受到了挑战，在便携安装模式下，我们将 Baulk 的软件安装到程序所在的相关目录，这样的好处是，如果用户想要删除 Baulk 可以删除整个目录即可。同时升级可以直接运行 `baulk-update` 命令，非常简单，但如果 Baulk 通过系统安装或者发布到 Windows Store 按照 Windows Store 的方式升级，则可能会导致安装绿色软件报告无权限写入或者绿色软件随安装卸载，我们要解决这一问题且不影响目前 Baulk 的便携能力，则需要设计一个好的 Baulk 虚拟文件系统，将 Baulk 的操作都通过虚拟路径进行。

在 Baulk 中，我们先建立一些关键的路径，这些路径可能会在支持 VFS 的包中进行推导。

|VFS 虚拟路径|调用函数|说明|是否导出|
|---|---|---|---|
|`BAULK_ROOT`|`baulk::vfs::AppRoot()`|Baulk 包的展开目录|导出|
|`BAULK_APPDATA`|`baulk::vfs::AppData()`|Baulk 便携配置目录|导出|
|`BAULK_VFS`|`baulk::vfs::AppVFS()`|该目录通常用于 NodeJS Rust Golang 等支持 venv 特性的包，覆盖默认的环境变量，避免不同的包数据相互干扰，该目录除非是运行强制删除，否则删除包的时候不会删除|导出|
|`BAULK_PKGROOT`|...|该变量与特定的包相关|导出|
|`BAULK_BINDIR`|...|||

在使用全新安装的 Baulk 时，将获得如下新的布局：

```txt
baulk
├── appdata
│   ├── belautils
│   │   └── kisasum.pos.json
│   └── wsudo
│       └── Privexec.json
├── baulk.env
├── buckets
│   └── baulk
├── local
│   └── bin
│       ├── baulk.linkmeta.json
│       └── curl.exe
├── locks
│   └── curl.json
├── packages
│   └── curl
│       └── bin
│           └── curl.exe
└── vfs
    ├── .cargo
    │   └── bin
    │       └── rustc.exe
    └── go
        └── bin
            └── gopls.exe
```

该布局不再与 baulk.exe 路径有过于密切的关联，这是因为要支持从安装包安装 Baulk 或者从 Windows Store 安装，注意，旧有的便携式升级会尽可能的保留原有的布局结构。

## Windows 11 上下文菜单支持

在前面我们要使用新的目录布局，这也是因为要支持 Windows Store。如果不是 Packaged App，似乎 Windows 11 Explorer 不会在新的上下文菜单中显示相应的条目。


## 更好的压缩解压

TODO

## 有限的 Scoop 包清单支持

1. 支持有限解析 Scoop 清单
2. 将 Scoop 清单转变为 Baulk bucket 清单。

## 更好的包管理和镜像支持

TODO

## 断点下载支持

TODO