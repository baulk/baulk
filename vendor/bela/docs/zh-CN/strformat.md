# StrFormat

Bela 目前提供了一个类型安全简单的 `StrFormat`, `StrFormat` 基于 C++ 变参模板，使用 `union` 记录参数类型，在解析时按照输入的占位符将其他类型转换为字符串连接在一起，从而实现格式化功能。

支持的类型和响应的占位符如下表所示：

|类型|占位符|备注|
|---|---|---|
|char|`%c`|ASCII 字符，会被提升为 wchar_t|
|unsigned char|`%c`|ASCII 字符，会被提升为 wchar_t|
|wchar_t|`%c`|UTF-16 字符|
|char16_t|`%c`|UTF-16 字符|
|char32_t|`%c`|UTF-32 Unicode 字符，会被转为 UTF-16 字符，这意味着可以使用 Unicode 码点以 %c 的方式输出 emoji。|
|short|`%d`|16位整型|
|unsigned short|`%d`|16位无符号整型|
|int|`%d`|32位整型|
|unsigned int|`%d`|32位无符号整型|
|long|`%d`|32位整型|
|unsigned long|`%d`|32位无符号整型|
|long long|`%d`|64位整型|
|unsigned long long|`%d`|64位无符号整型|
|float|`%f`|会被提升为 `double`|
|double|`%f`|64位浮点|
|const char *|`%s`|UTF-8 字符串，会被转换成 UTF-16 字符串|
|char *|`%s`|UTF-8 字符串，会被转换成 UTF-16 字符串|
|std::string|`%s`|UTF-8 字符串，会被转换成 UTF-16 字符串|
|std::string_view|`%s`|UTF-8 字符串，会被转换成 UTF-16 字符串|
|const wchar_t *|`%s`|UTF-16 字符串|
|wchar_t *|`%s`|UTF-16 字符串|
|std::wstring|`%s`|UTF-16 字符串|
|std::wstring_view|`%s`|UTF-16 字符串|
|const char16_t *|`%s`|UTF-16 字符串|
|char16_t *|`%s`|UTF-16 字符串|
|std::u16string|`%s`|UTF-16 字符串|
|std::u16string_view|`%s`|UTF-16 字符串|
|void *|`%p`|指针类型，会格式化成 `0xffff00000` 这样的字符串|

如果不格式化 UTF-8 字符串，且拥有固定大小内存缓冲区，可以使用 `StrFormat` 的如下重载，此重载可以轻松的移植到 POSIX 系统并支持异步信号安全:

```c++
template <typename... Args>
ssize_t StrFormat(wchar_t *buf, size_t N, const wchar_t *fmt, const Args& ... args)
```

## 示例

```cpp
///
#include <bela/str_cat.hpp>
#include <bela/stdwriter.hpp>

int wmain(int argc, wchar_t **argv) {
  auto ux = "\xf0\x9f\x98\x81 UTF-8 text \xE3\x8D\xA4"; // force encode UTF-8
  wchar_t wx[] = L"Engine \xD83D\xDEE0 中国";
  bela::FPrintF(
      stderr,
      L"Argc: %d Arg0: \x1b[32m%s\x1b[0m W: %s UTF-8: %s __cplusplus: %d\n", argc, argv[0], wx, ux, __cplusplus);
  char32_t em = 0x1F603;//😃
  auto s = bela::StringCat(L"Look emoji -->", em, L" U: ",
                           static_cast<uint32_t>(em));
  bela::FPrintF(stderr, L"emoji test %c %s\n", em, s);
  return 0;
}

```

请注意，如果上述 emoji 要正常显示，应当使用 `Windows Terminal` 或者是 `Mintty`。
