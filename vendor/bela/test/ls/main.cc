#include <bela/fs.hpp>
#include <bela/terminal.hpp>
#include <bela/path.hpp>

int wmain(int argc, wchar_t **argv) {
  std::wstring dir = L".";
  std::wstring baseName = L"*";
  if (argc >= 2) {
    dir = argv[1];
  }
  bela::error_code ec;
  bela::fs::Finder finder;
  if ((GetFileAttributesW(dir.data()) & FILE_ATTRIBUTE_DIRECTORY) == 0) {
    if (!finder.First(dir, ec)) {
      bela::FPrintF(stderr, L"List error %v\n", ec);
      return 1;
    }
  } else {
    if (!finder.First(dir, L"*", ec)) {
      bela::FPrintF(stderr, L"List error %v\n", ec);
      return 1;
    }
  }
  do {
    if (finder.Ignore()) {
      continue;
    }
    if (finder.IsDir()) {
      bela::FPrintF(stderr, L"D          0 %s\n", finder.Name());
      continue;
    }
    bela::FPrintF(stderr, L"F %10d %s\n", finder.Size(), finder.Name());
  } while (finder.Next());
  return 0;
}