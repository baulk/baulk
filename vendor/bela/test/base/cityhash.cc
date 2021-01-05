//
#include <bela/city.hpp>
#include <bela/terminal.hpp>

int wmain(int argc, wchar_t **argv) {
  std::wstring_view sn(argv[0]);
  bela::FPrintF(stderr, L"%s --> hash: %d\n", argv[0],
                bela::CityHash64(reinterpret_cast<const char *>(sn.data()), sn.size() * sizeof(wchar_t)));
  return 0;
}