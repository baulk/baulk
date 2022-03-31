////
#include <bela/charconv.hpp>
#include <bela/str_cat.hpp>
#include <bela/terminal.hpp>
#include <charconv>

[[maybe_unused]] constexpr uint64_t KB = 1024ULL;
[[maybe_unused]] constexpr uint64_t MB = KB * 1024;
[[maybe_unused]] constexpr uint64_t GB = MB * 1024;
[[maybe_unused]] constexpr uint64_t TB = GB * 1024;

inline std::wstring_view to_chars(wchar_t *buf, size_t len, double v, wchar_t la) {
  if (auto result = bela::to_chars(buf, buf + len, v, std::chars_format::fixed, 2); result.ec == std::errc{}) {
    if (result.ptr < buf + len) {
      *result.ptr++ = la;
    }
    return std::wstring_view{buf, static_cast<size_t>(result.ptr - buf)};
  }
  return L"";
}

inline std::wstring_view encode_rate(wchar_t *buf, size_t len, uint64_t x) {
  if (x >= TB) {
    return to_chars(buf, len, (double)x / TB, 'T');
  }
  if (x >= GB) {
    return to_chars(buf, len, (double)x / GB, 'G');
  }
  if (x >= MB) {
    return to_chars(buf, len, (double)x / MB, 'M');
  }
  if (x > 10 * KB) {
    return to_chars(buf, len, (double)x / KB, 'K');
  }
  if (auto result = bela::to_chars(buf, buf + len, x); result.ec == std::errc{}) {
    if (result.ptr < buf + len) {
      *result.ptr++ = 'B';
    }
    return std::wstring_view{buf, static_cast<size_t>(result.ptr - buf)};
  }
  return L"";
}

int wmain() {
  const wchar_t *w = L"196.1082741";
  double n;

  auto r = bela::from_chars(w, w + wcslen(w), n);
  const char *nums[] = {"123", "456.3", "75815278", "123456", "0x1233", "0123456", "x0111", "-78152"};
  for (const auto s : nums) {
    int X = 0;
    if (bela::SimpleAtoi(s, &X)) {
      bela::FPrintF(stderr, L"Good integer %s --> %d\n", n, X);
      continue;
    }
    bela::FPrintF(stderr, L"Bad integer %s\n", n);
  }

  bela::FPrintF(stderr, L"%0.2f\n", n);
  wchar_t buffer[64] = {0};
  bela::to_chars(buffer, buffer + 64, n, std::chars_format::fixed, 2);
  bela::FPrintF(stderr, L"buffer: %v\n", buffer);
  int64_t sizes[] = {
      1024, 1025, 1026 * 2048, TB * 2, GB * 3, GB + MB * 50, 75815278,
  };
  for (const auto s : sizes) {
    bela::FPrintF(stderr, L"encode rate: %v\n", encode_rate(buffer, 64, s));
  }

  return 0;
}
