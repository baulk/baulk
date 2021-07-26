//
#include <bela/base.hpp>
#include <bela/terminal.hpp>
#include <bela/codecvt.hpp>
#include <bela/path.hpp>

int32_t OnEntry(void *handle, void *userdata, const char *path) {
  auto termsz = reinterpret_cast<const bela::terminal::terminal_size *>(userdata);
  if (termsz == nullptr || termsz->columns <= 8) {
    return 0;
  }
  auto suglen = static_cast<size_t>(termsz->columns) - 8;
  auto filename = bela::ToWide(std::string_view{path});
  if (auto n = bela::string_width<wchar_t>(filename); n <= suglen) {
    bela::FPrintF(stderr, L"\x1b[33mx %s\x1b[0m\n", filename);
    return 0;
  }
  auto basename = bela::BaseName(filename);
  auto n = bela::string_width<wchar_t>(basename);
  if (n <= suglen) {
    bela::FPrintF(stderr, L"\x1b[33mx ...\\%s\x1b[0m\n", basename);
    return 0;
  }
  bela::FPrintF(stderr, L"\x1b[33mx ...%s\x1b[0m\n", basename.substr(n - suglen));
  return 0;
}

int wmain() {
  bela::terminal::terminal_size termsz{80, 20};
  const char *paths[] = {
      "example/hello.txt",
      R"(C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Tools\MSVC\14.26.28801\bin\Hostx64\x64\cl.exe)",
      R"(C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Tools\MSVC\14.26.28801\include\string_view)",
      R"(C-Program-Files-x86-Microsoft-Visual-Studio-2019-Community-VC-Tools-MSVC-14.26.28801-include-string_view)"};
  for (auto p : paths) {
    OnEntry(nullptr, &termsz, p);
  }
  return 0;
}