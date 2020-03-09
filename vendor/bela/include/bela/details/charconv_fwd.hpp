// Porting from MSVC STL
#ifndef BELA_DETAILS_CHARCONV_FWD_HPP
#define BELA_DETAILS_CHARCONV_FWD_HPP
#include <type_traits>
#include <system_error>
#include <cstring>
#include <cstdint>

#ifdef __has_attribute
#define BELA_HAVE_ATTRIBUTE(x) __has_attribute(x)
#else
#define BELA_HAVE_ATTRIBUTE(x) 0
#endif

#if BELA_HAVE_ATTRIBUTE(always_inline) ||                                      \
    (defined(__GNUC__) && !defined(__clang__))
#define BELA_ATTRIBUTE_ALWAYS_INLINE __attribute__((always_inline))
#else
#define BELA_ATTRIBUTE_ALWAYS_INLINE
#endif

#if defined(_MSC_VER)
// We can achieve something similar to attribute((always_inline)) with MSVC by
// using the __forceinline keyword, however this is not perfect. MSVC is
// much less aggressive about inlining, and even with the __forceinline keyword.
#define BELA_BASE_INTERNAL_FORCEINLINE __forceinline
#else
// Use default attribute inline.
#define BELA_BASE_INTERNAL_FORCEINLINE inline BELA_ATTRIBUTE_ALWAYS_INLINE

#endif

namespace bela {
enum class chars_format {
  scientific = 0b001,
  fixed = 0b010,
  hex = 0b100,
  general = fixed | scientific,
};
[[nodiscard]] constexpr chars_format operator&(chars_format L,
                                               chars_format R) noexcept {
  using I = std::underlying_type_t<chars_format>;
  return static_cast<chars_format>(static_cast<I>(L) & static_cast<I>(R));
}
[[nodiscard]] constexpr chars_format operator|(chars_format L,
                                               chars_format R) noexcept {
  using I = std::underlying_type_t<chars_format>;
  return static_cast<chars_format>(static_cast<I>(L) | static_cast<I>(R));
}
struct to_chars_result {
  wchar_t *ptr;
  std::errc ec;
};
template <typename T> T _Min_value(T a, T b) { return a < b ? a : b; }
template <typename T> T _Max_value(T a, T b) { return a > b ? a : b; }
template <typename T> void ArrayCopy(void *dst, const T *src, size_t n) {
  memcpy(dst, src, n * sizeof(T));
}
template <typename T> void Memset(T *buf, T ch, size_t n) {
  std::fill_n(buf, n, ch);
}

} // namespace bela

#endif
