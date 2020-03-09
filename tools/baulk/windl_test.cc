#include <bela/base.hpp>
#include <bela/stdwriter.hpp>

namespace baulk {
std::optional<std::wstring> WinGetInternal(std::wstring_view url,
                                           std::wstring_view workdir,
                                           bool forceoverwrite,
                                           bela::error_code ec);
} // namespace baulk

int wmain(int argc, wchar_t **argv) {
  if (argc < 2) {
    bela::FPrintF(stderr, L"usage: %s url\n", argv[0]);
    return 1;
  }
  bela::error_code ec;
  auto file = baulk::WinGetInternal(argv[1], L".", true, ec);
  if (!file) {
    bela::FPrintF(stderr, L"download: %s error: %s\n", argv[1], ec.message);
    return 1;
  }
  bela::FPrintF(stderr, L"download: %s => %s\n", argv[1], *file);
  return 0;
}