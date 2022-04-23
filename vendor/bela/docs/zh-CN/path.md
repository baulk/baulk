# Windows Path Utility

## PathCat 函数

`PathCat`,`JoinPath` 函数借鉴了 `StringCat` 函数，将路径组件连接起来。例子如下：

```c++
  auto p = bela::PathCat(L"\\\\?\\C:\\Windows/System32", L"drivers/etc", L"hosts");
  bela::FPrintF(stderr, L"PathCat: %s\n", p);
  auto p2 = bela::PathCat(L"C:\\Windows/System32", L"drivers/../..");
  bela::FPrintF(stderr, L"PathCat: %s\n", p2);
  auto p3 = bela::PathCat(L"Windows/System32", L"drivers/./././.\\.\\etc");
  bela::FPrintF(stderr, L"PathCat: %s\n", p3);
  auto p4 = bela::PathCat(L".", L"test/pathcat/./pathcat_test.exe");
  bela::FPrintF(stderr, L"PathCat: %s\n", p4);
  auto p5 = bela::PathCat(L"C:\\Windows\\System32\\drivers\\..\\.\\IME");
  bela::FPrintF(stderr, L"PathCat: %s\n", p5);
```
`PathCat` 的思路是获得 root 后，将其他路径按路径分隔符拆分成数组然后合成，`JoinPath` 与 `PathCat` 不一致的地方在于，`JoinPath` 将会把路径参数中的第一个解析为绝对路径，然后再参与计算。

`PathCat`，`JoinPath` 并不会判断路径是否存在，因此需要注意。

路径解析错误是很多软件的漏洞根源，合理的规范化路径非常有必要，而 `PathCat` 在规范化路径时，使用 C++17/C++20(Span) 的特性，减少内存分配，简化了规范化流程。


## PathExists 函数

`PathExists` 函数判断路径是否存在，当使用默认参数时，只会判断路径是否存在，如果需要判断路径的其他属性，可以使用如下方式：

```c++
if(!bela::PathExists(L"C:\\Windows",FileAttribute::Dir)){
    bela::FPrintF(stderr,L"C:\\Windows not dir or not exists\n");
}
```

## LookupRealPath 函数

`LookupRealPath` 用于解析 Windows 符号链接和卷挂载点。

## LookupAppExecLinkTarget 函数

`LookupAppExecLinkTarget` 用于解析 Windows AppExecLink 目标，在 Windows 10 系统中，`AppExecLink` 是一种 Store App 的命令行入口，通常位于 `C:\Users\$Username\AppData\Local\Microsoft\WindowsApps`，这种文件本质上是一种重解析点，因此解析时需要按照重解析点的方法去解析。
