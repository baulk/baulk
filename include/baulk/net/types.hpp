//
#ifndef BAULK_DETAILS_NET_INTERNAL_HPP
#define BAULK_DETAILS_NET_INTERNAL_HPP
#include <bela/base.hpp>
#include <bela/match.hpp>
#include <bela/ascii.hpp>
#include <bela/phmap.hpp>
#include <bela/path.hpp>
#include <memory>

namespace baulk::net {

namespace net_internal {
// hasher
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

constexpr bool hash_decode(const char *hash_text, size_t length, uint8_t *buffer, size_t size) {
  auto ms = (std::min)(length / 2, size);
  for (size_t i = 0; i < ms; i++) {
    auto b = net_internal::decode_byte_couple(static_cast<uint8_t>(hash_text[i * 2]),
                                              static_cast<uint8_t>(hash_text[i * 2 + 1]));
    if (b < 0) {
      return false;
    }
    buffer[i] = static_cast<uint8_t>(b);
  }
  return true;
}

constexpr bool hash_decode(const wchar_t *hash_text, size_t length, uint8_t *buffer, size_t size) {
  auto ms = (std::min)(length / 2, size);
  for (size_t i = 0; i < ms; i++) {
    auto b = net_internal::decode_byte_couple(static_cast<uint8_t>(hash_text[i * 2]),
                                              static_cast<uint8_t>(hash_text[i * 2 + 1]));
    if (b < 0) {
      return false;
    }
    buffer[i] = static_cast<uint8_t>(b);
  }
  return true;
}

constexpr bool hash_decode(std::string_view hash_text, uint8_t *buffer, size_t size) {
  return hash_decode(hash_text.data(), hash_text.size(), buffer, size);
}

constexpr bool hash_decode(std::wstring_view hash_text, uint8_t *buffer, size_t size) {
  return hash_decode(hash_text.data(), hash_text.size(), buffer, size);
}

template <size_t N> bool hash_decode(std::wstring_view hash_text, uint8_t (&buffer)[N]) {
  return hash_decode(hash_text.data(), hash_text.size(), buffer, N);
}

template <typename T, typename U, size_t N> bool bytes_equal(const T (&a)[N], const U (&b)[N]) {
  return std::equal(a, a + N, b);
}

using headers_t = bela::flat_hash_map<std::wstring, std::wstring, net_internal::StringCaseInsensitiveHash,
                                      net_internal::StringCaseInsensitiveEq>;

enum class protocol_version { HTTP11, HTTP2, HTTP3 };
struct minimal_response {
  headers_t headers;
  unsigned long status_code{0};
  protocol_version version{protocol_version::HTTP11};
  std::wstring status_text;
  [[nodiscard]] bool IsSuccessStatusCode() const { return status_code >= 200 && status_code <= 299; }
};

std::string url_decode(std::wstring_view url);

inline std::wstring url_path_name(std::wstring_view urlpath) {
  std::vector<std::wstring_view> pv = bela::SplitPath(urlpath);
  if (pv.empty()) {
    return L"index.html";
  }
  return std::wstring(pv.back());
}

inline std::wstring decoded_url_path_name(std::wstring_view urlpath) {
  std::vector<std::wstring_view> pv = bela::SplitPath(urlpath);
  if (pv.empty()) {
    return L"index.html";
  }
  auto name = pv.back();
  if (name.find(L'%') == std::wstring_view::npos) {
    return std::wstring(name);
  }
  return bela::encode_into<char, wchar_t>(url_decode(pv.back()));
}

} // namespace baulk::net

#endif