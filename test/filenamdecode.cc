//
#include <string>
#include <cstdio>

/* clang-format off */
  constexpr char unreservedtable[]={
     -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
     -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
     -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,'-','.',-1, // - .
     '0','1','2','3','4','5','6','7','8','9',-1,-1,-1,-1,-1,-1, // 0-9
     -1,'A','B','C','D','E','F','G','H','I','J','K','L','M','N','O', // A-Z
     'P','Q','R','S','T','U','V','W','X','Y','Z',-1,-1,-1,-1,'_', // _
     -1,'a','b','c','d','e','f','g','h','i','j','k','l','m','n','o', // a-z ~
     'p','q','r','s','t','u','v','w','x','y','z',-1,-1,-1,'~',-1,
     -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
     -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
     -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
     -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
     -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
     -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
     -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
     -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1
   };
/* clang-format on */

inline bool isunreserved(unsigned char ch) { return unreservedtable[ch] != -1; }

inline std::string EscapeURI(std::string_view sv) {
  constexpr const char hex[] = "0123456789ABCDEF";
  std::string result;
  result.reserve(sv.size() * 2);
  for (auto c : sv) {
    auto ch = static_cast<unsigned char>(c);
    if (isunreserved(ch)) {
      result.push_back(c);
      continue;
    }
    result.push_back('%');
    result.push_back(hex[ch / 16]); // Max 255 so
    result.push_back(hex[ch % 16]);
  }
  return result;
}

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

std::string UrlDecode(std::string_view str) {
  std::string buf;
  buf.reserve(str.size());
  auto p = str.data();
  auto len = str.size();
  for (size_t i = 0; i < len; i++) {
    auto ch = p[i];
    if (ch != '%') {
      buf.push_back(ch);
      continue;
    }
    if (i + 3 > len) {
      return buf;
    }
    auto c1 = static_cast<unsigned char>(p[i + 1]);
    auto c2 = static_cast<unsigned char>(p[i + 2]);
    auto n = decodehlen2(c1, c2);
    if (n > 0) {
      buf.push_back(static_cast<char>(n));
    }
    i += 2;
  }
  return buf;
}

int main() {
  //
  fprintf(stderr, "%s\n", EscapeURI("💘.zip").data());
  constexpr char str[] = "💘%F0%9F%92%98.zip";
  auto n = UrlDecode(str);
  fprintf(stderr, "%s\n", n.data());
  return 0;
}
