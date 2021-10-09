//
#ifndef BAULK_DETAILS_NET_INTERNAL_HPP
#define BAULK_DETAILS_NET_INTERNAL_HPP
#include <bela/match.hpp>
#include <bela/ascii.hpp>
#include <bela/phmap.hpp>

namespace baulk::net::net_internal {
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
using headers_t = bela::flat_hash_map<std::wstring, std::wstring, StringCaseInsensitiveHash, StringCaseInsensitiveEq>;
} // namespace baulk::net::net_internal

#endif