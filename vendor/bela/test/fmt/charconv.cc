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

using std::chars_format;

struct DoublePrecisionToWideTestCase {
  double value;
  chars_format fmt;
  int precision;
  const wchar_t *correct;
};

struct DoubleToWideTestCase {
  double value;
  chars_format fmt;
  const wchar_t *correct;
};

struct FloatToWideTestCase {
  float value;
  chars_format fmt;
  const wchar_t *correct;
};

inline constexpr DoublePrecisionToWideTestCase wide_digit_pairs_test_cases[] = {
    {0.0001020304, chars_format::fixed, 10, L"0.0001020304"}, {0.0506070809, chars_format::fixed, 10, L"0.0506070809"},
    {0.1011121314, chars_format::fixed, 10, L"0.1011121314"}, {0.1516171819, chars_format::fixed, 10, L"0.1516171819"},
    {0.2021222324, chars_format::fixed, 10, L"0.2021222324"}, {0.2526272829, chars_format::fixed, 10, L"0.2526272829"},
    {0.3031323334, chars_format::fixed, 10, L"0.3031323334"}, {0.3536373839, chars_format::fixed, 10, L"0.3536373839"},
    {0.4041424344, chars_format::fixed, 10, L"0.4041424344"}, {0.4546474849, chars_format::fixed, 10, L"0.4546474849"},
    {0.5051525354, chars_format::fixed, 10, L"0.5051525354"}, {0.5556575859, chars_format::fixed, 10, L"0.5556575859"},
    {0.6061626364, chars_format::fixed, 10, L"0.6061626364"}, {0.6566676869, chars_format::fixed, 10, L"0.6566676869"},
    {0.7071727374, chars_format::fixed, 10, L"0.7071727374"}, {0.7576777879, chars_format::fixed, 10, L"0.7576777879"},
    {0.8081828384, chars_format::fixed, 10, L"0.8081828384"}, {0.8586878889, chars_format::fixed, 10, L"0.8586878889"},
    {0.9091929394, chars_format::fixed, 10, L"0.9091929394"}, {0.9596979899, chars_format::fixed, 10, L"0.9596979899"},
};

void double_to_chars_check() {
  wchar_t buffer[64];
  for (const auto &c : wide_digit_pairs_test_cases) {
    auto sv = bela::to_chars_view(buffer, c.value, c.fmt, c.precision);
    bela::FPrintF(stderr, L"want %s got %s\n", c.correct, sv);
  }
}

inline constexpr DoubleToWideTestCase double_to_wide_test_cases[] = {
    // Test special cases (zero, inf, nan) and an ordinary case. Also test negative signs.
    {0.0, chars_format::scientific, L"0e+00"},
    // {-0.0, chars_format::scientific, L"-0e+00"},
    // {double_inf, chars_format::scientific, L"inf"},
    // {-double_inf, chars_format::scientific, L"-inf"},
    // {double_nan, chars_format::scientific, L"nan"},
    // {-double_nan, chars_format::scientific, L"-nan(ind)"},
    // {double_nan_payload, chars_format::scientific, L"nan"},
    // {-double_nan_payload, chars_format::scientific, L"-nan"},
    {2.018, chars_format::scientific, L"2.018e+00"},
    // {-2.018, chars_format::scientific, L"-2.018e+00"},
    {0.2018, chars_format::scientific, L"2.018e-01"},
    // {-0.2018, chars_format::scientific, L"-2.018e-01"},

    // Ditto for fixed, which doesn't emit exponents.
    {0.0, chars_format::fixed, L"0"},
    // {-0.0, chars_format::fixed, L"-0"},
    // {double_inf, chars_format::fixed, L"inf"},
    // {-double_inf, chars_format::fixed, L"-inf"},
    // {double_nan, chars_format::fixed, L"nan"},
    // {-double_nan, chars_format::fixed, L"-nan(ind)"},
    // {double_nan_payload, chars_format::fixed, L"nan"},
    // {-double_nan_payload, chars_format::fixed, L"-nan"},
    {2.018, chars_format::fixed, L"2.018"},
    // {-2.018, chars_format::fixed, L"-2.018"},

    // Ditto for general, which selects fixed for the scientific exponent 0.
    {0.0, chars_format::general, L"0"},
    // {-0.0, chars_format::general, L"-0"},
    // {double_inf, chars_format::general, L"inf"},
    // {-double_inf, chars_format::general, L"-inf"},
    // {double_nan, chars_format::general, L"nan"},
    // {-double_nan, chars_format::general, L"-nan(ind)"},
    // {double_nan_payload, chars_format::general, L"nan"},
    // {-double_nan_payload, chars_format::general, L"-nan"},
    {2.018, chars_format::general, L"2.018"},
    // {-2.018, chars_format::general, L"-2.018"},

    // Ditto for plain, which selects fixed because it's shorter for these values.
    {0.0, chars_format{}, L"0"},
    // {-0.0, chars_format{}, L"-0"},
    // {double_inf, chars_format{}, L"inf"},
    // {-double_inf, chars_format{}, L"-inf"},
    // {double_nan, chars_format{}, L"nan"},
    // {-double_nan, chars_format{}, L"-nan(ind)"},
    // {double_nan_payload, chars_format{}, L"nan"},
    // {-double_nan_payload, chars_format{}, L"-nan"},
    {2.018, chars_format{}, L"2.018"},
    // {-2.018, chars_format{}, L"-2.018"},

    // Ditto for hex.
    // {0.0, chars_format::hex, L"0p+0"},
    // {-0.0, chars_format::hex, L"-0p+0"},
    // {double_inf, chars_format::hex, L"inf"},
    // {-double_inf, chars_format::hex, L"-inf"},
    // {double_nan, chars_format::hex, L"nan"},
    // {-double_nan, chars_format::hex, L"-nan(ind)"},
    // {double_nan_payload, chars_format::hex, L"nan"},
    // {-double_nan_payload, chars_format::hex, L"-nan"},
    // {0x1.729p+0, chars_format::hex, L"1.729p+0"},
    // {-0x1.729p+0, chars_format::hex, L"-1.729p+0"},
    // {0x1.729p-1, chars_format::hex, L"1.729p-1"},
    // {-0x1.729p-1, chars_format::hex, L"-1.729p-1"},
};

void double_scientific_check() {
  wchar_t buffer[64];
  for (const auto &c : double_to_wide_test_cases) {
    if (c.fmt == std::chars_format{}) {
      auto sv = bela::to_chars_view(buffer, c.value);
      bela::FPrintF(stderr, L"want %s got %s\n", c.correct, sv);
      continue;
    }
    auto sv = bela::to_chars_view(buffer, c.value, c.fmt);
    bela::FPrintF(stderr, L"want %s got %s\n", c.correct, sv);
  }
}

inline constexpr FloatToWideTestCase float_to_wide_test_cases[] = {
    // Test special cases (zero, inf, nan) and an ordinary case. Also test negative signs.
    {0.0f, chars_format::scientific, L"0e+00"},
    // {-0.0f, chars_format::scientific, L"-0e+00"},
    // {float_inf, chars_format::scientific, L"inf"},
    // {-float_inf, chars_format::scientific, L"-inf"},
    // {float_nan, chars_format::scientific, L"nan"},
    // {-float_nan, chars_format::scientific, L"-nan(ind)"},
    // {float_nan_payload, chars_format::scientific, L"nan"},
    // {-float_nan_payload, chars_format::scientific, L"-nan"},
    {2.018f, chars_format::scientific, L"2.018e+00"},
    // {-2.018f, chars_format::scientific, L"-2.018e+00"},
    {0.2018f, chars_format::scientific, L"2.018e-01"},
    // {-0.2018f, chars_format::scientific, L"-2.018e-01"},

    // Ditto for fixed, which doesn't emit exponents.
    {0.0f, chars_format::fixed, L"0"},
    // {-0.0f, chars_format::fixed, L"-0"},
    // {float_inf, chars_format::fixed, L"inf"},
    // {-float_inf, chars_format::fixed, L"-inf"},
    // {float_nan, chars_format::fixed, L"nan"},
    // {-float_nan, chars_format::fixed, L"-nan(ind)"},
    // {float_nan_payload, chars_format::fixed, L"nan"},
    // {-float_nan_payload, chars_format::fixed, L"-nan"},
    {2.018f, chars_format::fixed, L"2.018"},
    // {-2.018f, chars_format::fixed, L"-2.018"},

    // Ditto for general, which selects fixed for the scientific exponent 0.
    {0.0f, chars_format::general, L"0"},
    // {-0.0f, chars_format::general, L"-0"},
    // {float_inf, chars_format::general, L"inf"},
    // {-float_inf, chars_format::general, L"-inf"},
    // {float_nan, chars_format::general, L"nan"},
    // {-float_nan, chars_format::general, L"-nan(ind)"},
    // {float_nan_payload, chars_format::general, L"nan"},
    // {-float_nan_payload, chars_format::general, L"-nan"},
    {2.018f, chars_format::general, L"2.018"},
    // {-2.018f, chars_format::general, L"-2.018"},

    // Ditto for plain, which selects fixed because it's shorter for these values.
    {0.0f, chars_format{}, L"0"},
    // {-0.0f, chars_format{}, L"-0"},
    // {float_inf, chars_format{}, L"inf"},
    // {-float_inf, chars_format{}, L"-inf"},
    // {float_nan, chars_format{}, L"nan"},
    // {-float_nan, chars_format{}, L"-nan(ind)"},
    // {float_nan_payload, chars_format{}, L"nan"},
    // {-float_nan_payload, chars_format{}, L"-nan"},
    {2.018f, chars_format{}, L"2.018"},
    // {-2.018f, chars_format{}, L"-2.018"},

    // Ditto for hex.
    // {0.0f, chars_format::hex, L"0p+0"},
    // {-0.0f, chars_format::hex, L"-0p+0"},
    // {float_inf, chars_format::hex, L"inf"},
    // {-float_inf, chars_format::hex, L"-inf"},
    // {float_nan, chars_format::hex, L"nan"},
    // {-float_nan, chars_format::hex, L"-nan(ind)"},
    // {float_nan_payload, chars_format::hex, L"nan"},
    // {-float_nan_payload, chars_format::hex, L"-nan"},
    // {0x1.729p+0f, chars_format::hex, L"1.729p+0"},
    // {-0x1.729p+0f, chars_format::hex, L"-1.729p+0"},
    // {0x1.729p-1f, chars_format::hex, L"1.729p-1"},
    // {-0x1.729p-1f, chars_format::hex, L"-1.729p-1"},
};

void float_scientific_check() {
  wchar_t buffer[64];
  for (const auto &c : float_to_wide_test_cases) {
    if (c.fmt == std::chars_format{}) {
      auto sv = bela::to_chars_view(buffer, c.value);
      bela::FPrintF(stderr, L"want %s got %s\n", c.correct, sv);
      continue;
    }
    auto sv = bela::to_chars_view(buffer, c.value, c.fmt);
    bela::FPrintF(stderr, L"want %s got %s\n", c.correct, sv);
  }
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
  double_to_chars_check();
  double_scientific_check();
  float_scientific_check();
  return 0;
}
