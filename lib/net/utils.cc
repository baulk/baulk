//
#include <baulk/net/types.hpp>

namespace baulk::net::net_internal {
inline constexpr int dumphex(unsigned char ch) {
  constexpr int hexval_table[] = {
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, // 0~0f
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, // 10~1f
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, // 20~2f
      0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  -1, -1, -1, -1, -1, -1, // 30~3f
      -1, 10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1, // 40~4f
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, // 50~5f
      -1, 10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1, // 60~6f
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, // 70~7f
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, // 80~8f
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, // 90~9f
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, // a0~af
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, // b0~bf
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, // c0~cf
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, // d0~df
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, // e0~ef
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1  // f0~ff
  };
  return hexval_table[ch];
}

inline int decodehlen2(unsigned char ch0, unsigned char ch1) {
  auto c1 = dumphex(ch0);
  auto c2 = dumphex(ch1);
  if (c1 < 0 || c2 < 0) {
    return -1;
  }
  return ((c1 << 4) | c2);
}

// decode url
std::wstring url_decode(std::wstring_view str) {
  std::wstring buf;
  buf.reserve(str.size());
  auto p = str.data();
  auto len = str.size();
  for (size_t i = 0; i < len; i++) {
    if (auto ch = p[i]; ch != '%') {
      buf += static_cast<wchar_t>(ch);
      continue;
    }
    if (i + 3 > len) {
      return buf;
    }
    if (auto n = decodehlen2(static_cast<unsigned char>(p[i + 1]), static_cast<unsigned char>(p[i + 2])); n > 0) {
      buf += static_cast<wchar_t>(n);
    }
    i += 2;
  }
  return buf;
}

} // namespace baulk::net::net_internal