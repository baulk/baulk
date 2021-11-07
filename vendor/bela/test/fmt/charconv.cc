////
#include <bela/charconv.hpp>
#include <bela/str_cat.hpp>
#include <bela/terminal.hpp>

int wmain() {
  const wchar_t *w = L"196.1082741";
  double n;
  auto r = bela::from_chars(w, w + wcslen(w), n);
  const char *nums[] = {"123", "456.3", "75815278", "123456", "0x1233", "0123456", "x0111", "-78152"};
  for (const auto n : nums) {
    int X = 0;
    if (bela::SimpleAtoi(n, &X)) {
      bela::FPrintF(stderr, L"Good integer %s --> %d\n", n, X);
      continue;
    }
    bela::FPrintF(stderr, L"Bad integer %s\n", n);
  }

  bela::FPrintF(stderr, L"%0.2f\n", n);
  return 0;
}
