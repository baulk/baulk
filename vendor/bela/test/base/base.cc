#include <bela/base.hpp>
#include <bela/terminal.hpp>
#include <bela/str_cat_narrow.hpp>
#include <bela/win32.hpp>

int wmain(int argc, wchar_t **argv) {
  if (argc >= 2) {
    FILE *fd = nullptr;
    if (auto e = _wfopen_s(&fd, argv[1], L"rb"); e != 0) {
      auto ec = bela::make_stdc_error_code(e);
      bela::FPrintF(stderr, L"unable open: %s\n", ec);
      return 1;
    }
    fclose(fd);
  }
  bela::FPrintF(stderr, L"%s\n", bela::narrow::StringCat("H: ", bela::narrow::AlphaNum(bela::narrow::Hex(123456))));
  bela::FPrintF(stderr, L"EADDRINUSE: %s\nEWOULDBLOCK: %s\n", bela::make_stdc_error_code(EADDRINUSE).message,
                bela::make_stdc_error_code(EWOULDBLOCK).message);
  auto version = bela::windows::version();
  bela::FPrintF(stderr, L"%d.%d.%d %d.%d\n", version.major, version.minor, version.build, version.service_pack_major,
                version.service_pack_minor);
  return 0;
}