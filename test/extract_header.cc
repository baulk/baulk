#include <bela/env.hpp>
#include <bela/strip.hpp>
#include <bela/terminal.hpp>

namespace net_internal {
struct StringCaseInsensitiveHash {
  using is_transparent = void;
  std::size_t operator()(std::wstring_view wsv) const noexcept {
    /// See Wikipedia
    /// https://en.wikipedia.org/wiki/Fowler%E2%80%93Noll%E2%80%93Vo_hash_function
    // _WIN64 Defined as 1 when the compilation target is 64-bit ARM or x64. Otherwise, undefined.
#if defined(__x86_64) || defined(_WIN64)
    static_assert(sizeof(size_t) == 8, "This code is for 64-bit size_t.");
    constexpr size_t kFNVOffsetBasis = 14695981039346656037ULL;
    constexpr size_t kFNVPrime = 1099511628211ULL;
#else
    static_assert(sizeof(size_t) == 4, "This code is for 32-bit size_t.");
    constexpr size_t kFNVOffsetBasis = 2166136261U;
    constexpr size_t kFNVPrime = 16777619U;
#endif
    size_t val = kFNVOffsetBasis;
    std::string_view sv = {reinterpret_cast<const char *>(wsv.data()), wsv.size() * 2};
    for (auto c : sv) {
      val ^= static_cast<size_t>(bela::ascii_tolower(c));
      val *= kFNVPrime;
    }
    return val;
  }
};

struct StringCaseInsensitiveEq {
  using is_transparent = void;
  bool operator()(std::wstring_view wlhs, std::wstring_view wrhs) const { return bela::EqualsIgnoreCase(wlhs, wrhs); }
};
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

constexpr int decode_byte_couple(uint8_t a, uint8_t b) {
  auto a1 = hexval_table[a];
  auto b1 = hexval_table[b];
  if (a1 < 0 || b1 < 0) {
    return -1;
  }
  return ((a1 << 4) | b1);
}

} // namespace net_internal

std::string url_decode(std::wstring_view url) {
  auto u8url = bela::encode_into<wchar_t, char>(url);
  std::string buf;
  buf.reserve(u8url.size());
  auto p = u8url.data();
  auto len = u8url.size();
  for (size_t i = 0; i < len; i++) {
    if (auto ch = p[i]; ch != '%') {
      buf += static_cast<char>(ch);
      continue;
    }
    if (i + 3 > len) {
      return buf;
    }
    if (auto n = net_internal::decode_byte_couple(static_cast<uint8_t>(p[i + 1]), static_cast<uint8_t>(p[i + 2]));
        n > 0) {
      buf += static_cast<char>(n);
    }
    i += 2;
  }
  return buf;
}

using headers_t = bela::flat_hash_map<std::wstring, std::wstring, net_internal::StringCaseInsensitiveHash,
                                      net_internal::StringCaseInsensitiveEq>;

inline std::optional<std::wstring> resolve_filename(std::wstring_view es) {
  constexpr std::wstring_view fns = L"filename";
  constexpr std::wstring_view fnsu = L"filename*";
  constexpr std::wstring_view utf8 = L"UTF-8";
  auto s = bela::StripAsciiWhitespace(es);
  auto pos = s.find('=');
  if (pos == std::wstring_view::npos) {
    return std::nullopt;
  }
  auto field = bela::StripAsciiWhitespace(s.substr(0, pos));
  auto v = bela::StripAsciiWhitespace(s.substr(pos + 1));
  if (field == fns) {
    bela::ConsumePrefix(&v, L"\"");
    bela::ConsumeSuffix(&v, L"\"");
    return std::make_optional<>(std::wstring(v));
  }
  if (field != fnsu) {
    return std::nullopt;
  }
  if (pos = v.find(L"''"); pos == std::wstring_view::npos) {
    bela::ConsumePrefix(&v, L"\"");
    bela::ConsumeSuffix(&v, L"\"");
    return std::make_optional<>(std::wstring(v));
  }
  if (bela::EqualsIgnoreCase(v.substr(0, pos), utf8)) {
    auto name = v.substr(pos + 2);
    return std::make_optional<>(bela::encode_into<char, wchar_t>(url_decode(name)));
  }
  // unsupported encoding
  return std::nullopt;
}

// https://developer.mozilla.org/en-US/docs/Web/HTTP/Headers/Content-Disposition
// https://www.rfc-editor.org/rfc/rfc6266#section-5
inline std::optional<std::wstring> extract_filename(const headers_t &hkv) {
  auto it = hkv.find(L"Content-Disposition");
  if (it == hkv.end()) {
    return std::nullopt;
  }
  std::vector<std::wstring_view> pvv = bela::StrSplit(it->second, bela::ByChar(';'), bela::SkipEmpty());
  for (auto e : pvv) {
    if (auto result = resolve_filename(e); result) {
      return result;
    }
  }
  return std::nullopt;
}

int main() {
  headers_t h1;
  h1[L"Content-Disposition"] =
      L"attachment; filename*=UTF-8''%E5%8C%85%E7%AE%A1%E7%90%86%E5%99%A8%E4%B8%8B%E8%BD%BD.zip";
  if (auto name = extract_filename(h1); name) {
    bela::FPrintF(stderr, L"filename %s\n", *name);
  }
  h1[L"Content-Disposition"] = L"attachment; filename=\"baulk-6.0.zip\"";
  if (auto name = extract_filename(h1); name) {
    bela::FPrintF(stderr, L"filename %s\n", *name);
  }
  return 0;
}
