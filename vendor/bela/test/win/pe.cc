///
#include <bela/pe.hpp>
#include <bela/stdwriter.hpp>

int wmain(int argc, wchar_t **argv) {
  if (argc < 2) {
    bela::FPrintF(stderr, L"usage: %s pefile\n", argv[0]);
    return 1;
  }
  bela::error_code ec;
  auto pm = bela::pe::Expose(argv[1], ec);
  if (!pm) {
    bela::FPrintF(stderr, L"Unable parse pe file: %s\n", ec.message);
    return 1;
  }
  bela::FPrintF(stderr, L"File %s \nLinker: %s\nOS: %s\ndepends: \n", argv[1],
                pm->linkver.Str(), pm->osver.Str());
  for (const auto &d : pm->depends) {
    bela::FPrintF(stderr, L"    %s\n", d);
  }
  auto vi = bela::pe::ExposeVersion(argv[1], ec);
  if (!vi) {
    bela::FPrintF(stderr, L"ExposeVersion %s\n", ec.message);
    return 1;
  }
  bela::FPrintF(stderr, L"%s: %s\n", vi->FileDescription, vi->FileVersion);
  bela::FPrintF(stderr, L"Copyright: %s\n", vi->LegalCopyright);
  return 0;
}