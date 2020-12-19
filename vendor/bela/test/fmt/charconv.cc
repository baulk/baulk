////
#include <bela/charconv.hpp>
#include <bela/str_cat.hpp>
#include <bela/terminal.hpp>

int wmain() {
  const wchar_t *w = L"196.1082741";
  double n;
  auto r = bela::from_chars(w, w + wcslen(w), n);
  bela::FPrintF(stderr, L"%0.2f\n", n);
  return 0;
}
