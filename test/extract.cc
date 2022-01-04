///
#include <bela/terminal.hpp>
#include <baulk/archive.hpp>
using baulk::archive::file_format_t;
int wmain(int argc, wchar_t **argv) {
  if (argc < 2) {
    bela::FPrintF(stderr, L"usage: %s archive\n", argv[0]);
    return 1;
  }
  file_format_t afmt{file_format_t::none};
  int64_t offset{0};
  bela::error_code ec;
  auto fd = baulk::archive::OpenArchiveFile(argv[1], offset, afmt,ec);
  if (!fd) {
    bela::FPrintF(stderr, L"file %s not archive file: %s\n", argv[1], ec);
    return 1;
  }

  auto name = baulk::archive::FormatToMIME(afmt);
  if (offset != 0) {
    bela::FPrintF(stderr, L"file archive format: %s  (PE Self Extract offset: %d)\n", name, offset);
    return 0;
  }
  bela::FPrintF(stderr, L"file archive format: %s\n", name);
  return 0;
}