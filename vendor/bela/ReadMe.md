# Bela Utilities

[![license badge](https://img.shields.io/github/license/fcharlie/bela.svg)](LICENSE)
[![Master Branch Status](https://github.com/fcharlie/bela/workflows/BelaCI/badge.svg)](https://github.com/fcharlie/bela/actions)
[![Open in Visual Studio Code](https://open.vscode.dev/badges/open-in-vscode.svg)](https://open.vscode.dev/fcharlie/bela)

Modern C++20 library collection, better development experience on Windows (10,11)

A lot of the code supports wchar_t based on Abseil modifications, some of which come from Visual C++ STL.

## Feature

+   `bela::error_code` and `bela::make_system_error_code` `bela::make_stdc_error_code` `bela::from_std_error_code`
+   `bela::finally`
+   `bela::secure_wstring` `bela::secure_string`
+   `bela::bytes_view` bytes view
+   `bela::version` compare and manipulate versions are available as extensions to the `<major>.<minor>.<patch>.<build>-<prerelease_type>.<prerelease_number>`
+   `bela::StringCat` `bela::StrAppend` `bela::StrSplit` `bela::StrReplaceAll` `bela::StrJoin` `bela::Substitute`
+   `bela::hash` SHA256/SHA512 SHA3 BLAKE3 SM3
+   `bela::Time` `bela::DateTime`
+   `bela::Buffer`
+   `bela::EscapeArgv` escape `argv[]` to windows commandline
+   `bela::FPrintF` safely and unicode friendly printf impl
+   Support analyze zip format and list zip files
+   Support analyze PE file format
+   Support analyze ELF file format
+   Support analyze Mach-O file format
+   ...

## Used by

+   [Baulk - Minimal Package Manager for Windows](https://github.com/baulk/baulk)
+   [Privexec - Run the program with the specified permission level](https://github.com/M2Team/Privexec)
+   [BelaUtils - Tools reimplemented using Bela library](https://github.com/fcharlie/BelaUtils)

## Documents

Chinese docs: [zh-CN](./docs/zh-CN/)

## License

bela base [Abseil](https://github.com/abseil/abseil-cpp): strcat. numbers subsitute memutil match ascii str_replace str_join
bela base Visual C++: charconv (support wchar_t only)  
bela Use parallel-hashmap [https://github.com/greg7mdp/parallel-hashmap](https://github.com/greg7mdp/parallel-hashmap)  
bela Use semver [https://github.com/Neargye/semver](https://github.com/Neargye/semver)  
bela Use musl fnmatch(MIT) [https://github.com/bminor/musl/blob/master/src/regex/fnmatch.c](https://github.com/bminor/musl/blob/master/src/regex/fnmatch.c)  
bela Use LLVM tokenize commandline (support wchar_t only)  
bela Use [https://github.com/BLAKE3-team/BLAKE3](https://github.com/BLAKE3-team/BLAKE3)  
bela::hash uses the Hash algorithm code contributed by different people. You can check the corresponding file header for details.

[LICENSE](./LICENSE)