# Bela Utilities

A C++20 tool library that provides a better experience on the Windows 10 operating system.

A lot of the code supports wchar_t based on Abseil modifications, some of which come from Visual C++ STL.

## Feature

+   `bela::error_code` and `bela::make_system_error_code` `bela::make_stdc_error_code` `bela::from_std_error_code`
+   `bela::finally`
+   `bela::secure_wstring` `bela::secure_string`
+   `bela::StringCat` `bela::StrAppend`
+   `bela::hash` SHA256/SHA512 SHA3 BLAKE3 SM3
+   ...

## Documents

Chinese docs: [zh-CN](./docs/zh-CN/)

## License

bela base [Abseil](https://github.com/abseil/abseil-cpp): strcat. numbers subsitute memutil match ascii str_replace str_join (support wchar_t only)  
bela base Visual C++: charconv (support wchar_t only)  
bela Use parallel-hashmap [https://github.com/greg7mdp/parallel-hashmap](https://github.com/greg7mdp/parallel-hashmap)  
bela Use semver [https://github.com/Neargye/semver](https://github.com/Neargye/semver)  
bela TOML port from [https://github.com/skystrife/cpptoml](https://github.com/skystrife/cpptoml)  
bela Use musl fnmatch(MIT) [https://github.com/bminor/musl/blob/master/src/regex/fnmatch.c](https://github.com/bminor/musl/blob/master/src/regex/fnmatch.c)  
bela Use LLVM tokenize commandline (support wchar_t only)  
bela Use [https://github.com/BLAKE3-team/BLAKE3](https://github.com/BLAKE3-team/BLAKE3)  
bela::hash uses the Hash algorithm code contributed by different people. You can check the corresponding file header for details.

[LICENSE](./LICENSE)