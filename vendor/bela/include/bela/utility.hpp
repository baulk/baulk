//
#ifndef BELA_UTILITY_HPP
#define BELA_UTILITY_HPP
#include <intrin.h>
#include <bit>

#if (defined(_M_AMD64) || defined(__x86_64__)) || (defined(_M_ARM) || defined(__arm__))
#define _BELA_HAS_BITSCAN64
#define _BELA_64BIT 1
#endif

namespace bela {
[[noreturn]] inline void unreachable() {
  // Uses compiler specific extensions if possible.
  // Even if no extension is used, undefined behavior is still raised by
  // an empty function body and the noreturn attribute.
#ifdef __GNUC__ // GCC, Clang, ICC
  __builtin_unreachable();
#elif defined(_MSC_VER) // MSVC
  __assume(false);
#endif
}

// template <std::integral I> constexpr int __bela_popcount(I i) noexcept { return std::popcount(i); }

#ifndef _MSC_VER

inline constexpr int __bela_ctz(unsigned __x) noexcept { return __builtin_ctz(__x); }

inline constexpr int __bela_ctz(unsigned long __x) noexcept { return __builtin_ctzl(__x); }

inline constexpr int __bela_ctz(unsigned long long __x) noexcept { return __builtin_ctzll(__x); }

inline constexpr int __bela_clz(unsigned __x) noexcept { return __builtin_clz(__x); }

inline constexpr int __bela_clz(unsigned long __x) noexcept { return __builtin_clzl(__x); }

inline constexpr int __bela_clz(unsigned long long __x) noexcept { return __builtin_clzll(__x); }

#else // _LIBCPP_COMPILER_MSVC

// Precondition:  __x != 0
inline int __bela_ctz(unsigned __x) {
  static_assert(sizeof(unsigned) == sizeof(unsigned long), "");
  static_assert(sizeof(unsigned long) == 4, "");
  unsigned long __where;
  if (_BitScanForward(&__where, __x))
    return static_cast<int>(__where);
  return 32;
}

inline int __bela_ctz(unsigned long __x) {
  static_assert(sizeof(unsigned long) == sizeof(unsigned), "");
  return __bela_ctz(static_cast<unsigned>(__x));
}

inline int __bela_ctz(unsigned long long __x) {
  unsigned long __where;
#if defined(_BELA_HAS_BITSCAN64)
  if (_BitScanForward64(&__where, __x))
    return static_cast<int>(__where);
#else
  // Win32 doesn't have _BitScanForward64 so emulate it with two 32 bit calls.
  if (_BitScanForward(&__where, static_cast<unsigned long>(__x)))
    return static_cast<int>(__where);
  if (_BitScanForward(&__where, static_cast<unsigned long>(__x >> 32)))
    return static_cast<int>(__where + 32);
#endif
  return 64;
}

// Precondition:  __x != 0
inline int __bela_clz(unsigned __x) {
  static_assert(sizeof(unsigned) == sizeof(unsigned long), "");
  static_assert(sizeof(unsigned long) == 4, "");
  unsigned long __where;
  if (_BitScanReverse(&__where, __x))
    return static_cast<int>(31 - __where);
  return 32; // Undefined Behavior.
}

inline int __bela_clz(unsigned long __x) {
  static_assert(sizeof(unsigned) == sizeof(unsigned long), "");
  return __bela_clz(static_cast<unsigned>(__x));
}

inline int __bela_clz(unsigned long long __x) {
  unsigned long __where;
#if defined(_BELA_HAS_BITSCAN64)
  if (_BitScanReverse64(&__where, __x))
    return static_cast<int>(63 - __where);
#else
  // Win32 doesn't have _BitScanReverse64 so emulate it with two 32 bit calls.
  if (_BitScanReverse(&__where, static_cast<unsigned long>(__x >> 32)))
    return static_cast<int>(63 - (__where + 32));
  if (_BitScanReverse(&__where, static_cast<unsigned long>(__x)))
    return static_cast<int>(63 - __where);
#endif
  return 64; // Undefined Behavior.
}

#endif // _LIBCPP_COMPILER_MSVC

} // namespace bela

#endif