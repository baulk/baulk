/// DateTime format
#include <bela/datetime.hpp>
#include <bela/str_cat.hpp>
#include <bela/terminal.hpp>

namespace bela {

const wchar_t kDigits[] = L"0123456789";
const int kDigits10_64 = 18;
// Formats a 64-bit integer in the given field width.  Note that it is up
// to the caller of Format64() [and Format02d()/FormatOffset()] to ensure
// that there is sufficient space before ep to hold the conversion.
wchar_t *Format64(wchar_t *ep, int width, std::int_fast64_t v) {
  bool neg = false;
  if (v < 0) {
    --width;
    neg = true;
    if (v == (std::numeric_limits<std::int_fast64_t>::min)()) {
      // Avoid negating minimum value.
      std::int_fast64_t last_digit = -(v % 10);
      v /= 10;
      if (last_digit < 0) {
        ++v;
        last_digit += 10;
      }
      --width;
      *--ep = kDigits[last_digit];
    }
    v = -v;
  }
  do {
    --width;
    *--ep = kDigits[v % 10];
  } while (v /= 10);
  while (--width >= 0) {
    *--ep = '0'; // zero pad
  }
  if (neg) {
    *--ep = '-';
  }
  return ep;
}

// Formats [0 .. 99] as %02d.
wchar_t *Format02d(wchar_t *ep, int v) {
  *--ep = kDigits[v % 10];
  *--ep = kDigits[(v / 10) % 10];
  return ep;
}

inline std::wstring_view trimSuffixZero(std::wstring_view sv) {
  if (auto pos = sv.find_last_not_of('0'); pos != std::wstring_view::npos) {
    return sv.substr(0, pos + 1);
  }
  return sv;
}

// ABSL_DLL extern const char RFC3339_full[] = "%Y-%m-%d%ET%H:%M:%E*S%Ez";
// ABSL_DLL extern const char RFC3339_sec[] = "%Y-%m-%d%ET%H:%M:%S%Ez";
std::wstring DateTime::Format(bool nano) {
  std::wstring stime;
  constexpr size_t bufsize = sizeof("2020-12-06T18:12:47.9858521+08:00") + 6;
  stime.reserve(bufsize);
  // Scratch buffer for internal conversions.
  wchar_t buf[3 + kDigits10_64]; // enough for longest conversion
  wchar_t *const ep = buf + sizeof(buf) / 2;
  wchar_t *bp; // works back from ep

  bp = Format64(ep, 4, year);
  stime.append(bp, static_cast<std::size_t>(ep - bp)).push_back('-');
  bp = Format02d(ep, month);
  stime.append(bp, static_cast<std::size_t>(ep - bp)).push_back('-');
  bp = Format02d(ep, day);
  stime.append(bp, static_cast<std::size_t>(ep - bp)).push_back('T');
  bp = Format02d(ep, hour);
  stime.append(bp, static_cast<std::size_t>(ep - bp)).push_back(':');
  bp = Format02d(ep, minute);
  stime.append(bp, static_cast<std::size_t>(ep - bp)).push_back(':');
  bp = Format02d(ep, second);
  stime.append(bp, static_cast<std::size_t>(ep - bp));
  if (nano) {
    bela::AlphaNum an(nsec);
    auto sv = trimSuffixZero(an.Piece());
    if (!sv.empty()) {
      stime.append(L".").append(sv);
    }
  }
  if (tzoffset != 0) {
    stime.push_back((tzoffset > 0) ? '-' : '+');
    auto h = std::abs(tzoffset) / 3600;
    auto m = std::abs(tzoffset) % 3600 / 60;
    bp = Format02d(ep, h);
    stime.append(bp, static_cast<std::size_t>(ep - bp)).push_back(':');
    bp = Format02d(ep, m);
    stime.append(bp, static_cast<std::size_t>(ep - bp));
  } else {
    stime.push_back('Z');
  }
  return stime;
}

} // namespace bela