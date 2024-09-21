/// DateTime format
#include <bela/datetime.hpp>
#include <bela/terminal.hpp>

namespace bela::time_internal {
constexpr char kDigits[] = "0123456789";
constexpr int kDigits10_64 = 18;
// Formats a 64-bit integer in the given field width.  Note that it is up
// to the caller of Format64() [and Format02d()/FormatOffset()] to ensure
// that there is sufficient space before ep to hold the conversion.

template <typename CharT> CharT *Format64(CharT *ep, int width, std::int_fast64_t v) {
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
      *--ep = static_cast<CharT>(kDigits[last_digit]);
    }
    v = -v;
  }
  do {
    --width;
    *--ep = static_cast<CharT>(kDigits[v % 10]);
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
template <typename CharT> constexpr CharT *Format02d(CharT *ep, int v) {
  *--ep = static_cast<CharT>(kDigits[v % 10]);
  *--ep = static_cast<CharT>(kDigits[(v / 10) % 10]);
  return ep;
}

template <typename T> std::basic_string_view<T> trimZero(std::basic_string_view<T> sv) {
  if (auto pos = sv.find_first_not_of('0'); pos != std::basic_string_view<T>::npos) {
    sv.remove_prefix(pos);
  }
  if (auto pos = sv.find_last_not_of('0'); pos != std::basic_string_view<T>::npos) {
    return sv.substr(0, pos + 1);
  }
  return sv;
}

template <typename CharT = wchar_t, typename Allocator = std::allocator<CharT>>
  requires bela::character<CharT>
[[nodiscard]] std::basic_string<CharT, std::char_traits<CharT>, Allocator>
FormatDateTimeInternal(std::int_fast64_t year, bela::Month month, std::int_least8_t day, std::int_least8_t hour,
                       std::int_least8_t minute, std::int_least8_t second, std::int_least32_t nsec,
                       std::int_least32_t tzoffset, bool nano) {
  std::basic_string<CharT, std::char_traits<CharT>, Allocator> stime;
  constexpr size_t bufsize = sizeof("2020-12-06T18:12:47.9858521+08:00") + 6;
  stime.reserve(bufsize);
  // Scratch buffer for internal conversions.
  CharT buf[3 + kDigits10_64]; // enough for longest conversion
  CharT *const ep = buf + std::size(buf);
  CharT *bp; // works back from ep

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
    bp = Format64(ep, 10, nsec);
    auto sv = trimZero(std::basic_string_view<CharT>(bp, static_cast<size_t>(ep - bp)));
    if (!sv.empty()) {
      stime.push_back('.');
      stime.append(sv);
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

//  extern const char RFC3339_full[] = "%Y-%m-%d%ET%H:%M:%E*S%Ez";
//  extern const char RFC3339_sec[] = "%Y-%m-%d%ET%H:%M:%S%Ez";
std::wstring FormatDateTimeW(const DateTime &dt, bool nano) {
  return FormatDateTimeInternal<wchar_t>(dt.year, dt.month, dt.day, dt.hour, dt.minute, dt.second, dt.nsec, dt.tzoffset,
                                         nano);
}

std::string FormatDateTimeA(const DateTime &dt, bool nano) {
  return FormatDateTimeInternal<char>(dt.year, dt.month, dt.day, dt.hour, dt.minute, dt.second, dt.nsec, dt.tzoffset,
                                      nano);
}
} // namespace bela::time_internal