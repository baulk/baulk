////
#include <bela/charconv.hpp>
#include <bela/strcat.hpp>
#include <bela/stdwriter.hpp>

int wmain() {
  const wchar_t *w = L"196.1082741";
  double n;
  auto r = bela::from_chars(w, w + wcslen(w), n);
  bela::FPrintF(stderr, L"%0.2f\n", n);
  return 0;
}
