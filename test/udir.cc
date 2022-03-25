///
#include <bela/path.hpp>
#include <bela/match.hpp>
#include <bela/terminal.hpp>
#include <baulk/fs.hpp>
#include <filesystem>

int wmain(int argc, wchar_t **argv) {
  if (argc < 2) {
    bela::FPrintF(stderr, L"usage: %s folder\n", argv[0]);
    return 1;
  }
  std::error_code e;
  if (auto d = baulk::fs::Flattened(argv[1]); d) {
    bela::FPrintF(stderr, L"found flattened child folder: %v\n", d->native());
    if (std::filesystem::equivalent(argv[1], *d, e)) {
      bela::FPrintF(stderr, L"equivalent: %v %v\n", d->native(), argv[1]);
    }
    return 0;
  }
  bela::FPrintF(stderr, L"not flattened child folder: %v\n", argv[1]);
  return 0;
}