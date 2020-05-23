#include <bela/base.hpp>
#include <bela/terminal.hpp>

int wmain(int argc, wchar_t **argv) {
  if (argc >= 2) {
    FILE *fd = nullptr;
    if (auto e = _wfopen_s(&fd, argv[1], L"rb"); e != 0) {
      auto ec = bela::make_stdc_error_code(e);
      bela::FPrintF(stderr, L"unable open: %s\n", ec.message);
      return 1;
    }
    fclose(fd);
  }
  return 0;
}