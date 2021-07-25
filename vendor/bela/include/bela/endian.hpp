// endian.hpp
#ifndef BELA_ENDIAN_HPP
#define BELA_ENDIAN_HPP
#include <cstdint>
#include <cstring>
#include <type_traits>
#include <bit>
#include <span>
#include "types.hpp"

namespace bela {
// https://en.cppreference.com/w/cpp/language/inline
// A function declared constexpr is implicitly an inline function.
constexpr bool IsBigEndian() { return std::endian::native == std::endian::big; }
constexpr bool IsLittleEndian() { return std::endian::native == std::endian::little; }

constexpr uint16_t swap16(uint16_t value) {
  if (std::is_constant_evaluated()) {
    return (((value & uint16_t{0xFF}) << 8) | ((value & uint16_t{0xFF00}) >> 8));
  }
#if defined(_MSC_VER)
  // The DLL version of the runtime lacks these functions (bug!?), but in a
  // release build they're replaced with BSWAP instructions anyway.
  return _byteswap_ushort(value);
#else
  // defined(__llvm__) || (defined(__GNUC__) && !defined(__ICC))
  return __builtin_bswap16(src);
#endif
}
// We use C++17. so GCC version must > 8.0. __builtin_bswap32 awayls exists
constexpr uint32_t swap32(uint32_t value) {
  if (std::is_constant_evaluated()) {
    return (((value & uint32_t{0xFF}) << 24) | ((value & uint32_t{0xFF00}) << 8) | ((value & uint32_t{0xFF0000}) >> 8) |
            ((value & uint32_t{0xFF000000}) >> 24));
  }
#if defined(_MSC_VER)
  return _byteswap_ulong(value);
#else
  // defined(__llvm__) || (defined(__GNUC__) && !defined(__ICC))
  return __builtin_bswap32(value);
#endif
}

constexpr uint64_t swap64(uint64_t value) {
  if (std::is_constant_evaluated()) {
    return (((value & uint64_t{0xFF}) << 56) | ((value & uint64_t{0xFF00}) << 40) |
            ((value & uint64_t{0xFF0000}) << 24) | ((value & uint64_t{0xFF000000}) << 8) |
            ((value & uint64_t{0xFF00000000}) >> 8) | ((value & uint64_t{0xFF0000000000}) >> 24) |
            ((value & uint64_t{0xFF000000000000}) >> 40) | ((value & uint64_t{0xFF00000000000000}) >> 56));
  }
#if defined(_MSC_VER)
  return _byteswap_uint64(value);
#else
  // defined(__llvm__) || (defined(__GNUC__) && !defined(__ICC))
  return __builtin_bswap64(value);
#endif
}

constexpr unsigned char bswap(unsigned char v) { return v; }
constexpr signed char bswap(signed char v) { return v; }
constexpr unsigned short bswap(unsigned short v) { return swap16(v); }
constexpr signed short bswap(signed short v) {
  return static_cast<signed short>(swap16(static_cast<uint16_t>(v)));
}
constexpr unsigned int bswap(unsigned int v) {
  return static_cast<unsigned int>(swap32(static_cast<uint32_t>(v)));
}
constexpr signed int bswap(signed int v) { return static_cast<signed int>(swap32(static_cast<uint32_t>(v))); }

constexpr unsigned long bswap(unsigned long v) {
  static_assert(sizeof(unsigned long) == 8 || sizeof(unsigned long) == 4, "unsigned long size must 8 byte or 4 byte");
  if constexpr (sizeof(unsigned long) == 8) {
    return static_cast<unsigned long>(swap64(static_cast<uint64_t>(v)));
  }
  return static_cast<unsigned long>(swap32(static_cast<uint32_t>(v)));
}
constexpr signed long bswap(signed long v) {
  static_assert(sizeof(signed long) == 8 || sizeof(signed long) == 4, "signed long size must 8 byte or 4 byte");
  if constexpr (sizeof(signed long) == 8) {
    return static_cast<signed long>(swap64(static_cast<uint64_t>(v)));
  }
  return static_cast<signed long>(swap32(static_cast<uint32_t>(v)));
}

constexpr unsigned long long bswap(unsigned long long v) {
  return static_cast<unsigned long long>(swap64(static_cast<uint64_t>(v)));
}

constexpr signed long long bswap(signed long long v) {
  return static_cast<signed long long>(swap64(static_cast<uint64_t>(v)));
}

constexpr double bswap(double v) {
  union {
    uint64_t i;
    double d;
  } in, out;
  in.d = v;
  out.i = bswap(in.i);
  return out.d;
}

// SO Network order is BigEndian
template <typename T>
requires std::integral<T>
constexpr T htons(T v) {
  if constexpr (IsBigEndian()) {
    return v;
  }
  return bswap(v);
}
template <typename T>
requires std::integral<T>
constexpr T ntohs(T v) {
  if constexpr (IsBigEndian()) {
    return v;
  }
  return bswap(v);
}

template <typename T>
requires std::integral<T>
constexpr T fromle(T i) {
  if constexpr (IsBigEndian()) {
    return bswap(i);
  }
  return i;
}

template <typename T>
requires std::integral<T>
constexpr T frombe(T i) {
  if constexpr (IsBigEndian()) {
    return i;
  }
  return bswap(i);
}

template <typename T>
requires std::integral<T>
constexpr T unaligned_load(const void *p) {
  T t;
  if (std::is_constant_evaluated()) {
    auto p1 = reinterpret_cast<const uint8_t *>(p);
    auto p2 = reinterpret_cast<uint8_t *>(&t);
    for (size_t i = 0; i < sizeof(T); i++) {
      p2[i] = p1[i];
    }
    return t;
  }
  memcpy(&t, p, sizeof(T));
  return t;
}

template <typename T>
requires std::integral<T>
constexpr T cast_fromle(const void *p) {
  auto v = unaligned_load<T>(p);
  if constexpr (IsLittleEndian()) {
    return v;
  }
  return bswap(v);
}

template <typename T>
requires std::integral<T>
constexpr T cast_frombe(const void *p) {
  auto v = unaligned_load<T>(p);
  if constexpr (IsBigEndian()) {
    return v;
  }
  return bswap(v);
}

namespace endian {
template <std::endian E = std::endian::native> class Reader {
public:
  Reader() = default;
  template <typename T, size_t N>
  requires bela::standard_layout<T> Reader(T (&p)[N]) {
    data = reinterpret_cast<const uint8_t *>(&p);
    size = N * sizeof(T);
  }
  Reader(const void *p, size_t len) : data(reinterpret_cast<const uint8_t *>(p)), size(len) {}
  Reader(const Reader &other) {
    data = other.data;
    size = other.size;
  }
  Reader &operator=(const Reader &other) {
    data = other.data;
    size = other.size;
    return *this;
  }
  template <typename T = uint8_t>
  requires bela::standard_layout<T>
  void Reset(std::span<T> s) {
    data = reinterpret_cast<const uint8_t *>(s.data());
    size = s.size() * sizeof(T);
  }
  template <typename T>
  requires std::integral<T> T Read() {
    auto v = bela::unaligned_load<T>(data);
    data += sizeof(T);
    size -= sizeof(T);
    if constexpr (E == std::endian::native) {
      return v;
    }
    return bela::bswap(v);
  }
  size_t Discard(size_t n) {
    if (n >= size) {
      data = nullptr;
      size = 0;
      return 0;
    }
    size -= n;
    data += n;
    return size;
  }
  uint8_t Pick() {
    auto c = *data;
    data++;
    size--;
    return c;
  }
  Reader Sub(int n) {
    size += n;
    auto p = data;
    data += n;
    return Reader(p, n);
  }
  size_t Size() const { return size; }
  template <typename T = char>
  requires bela::standard_layout<T>
  const T *Data() const { return reinterpret_cast<const T *>(data); }

private:
  const uint8_t *data{nullptr};
  size_t size{0};
  const std::endian e{E};
};
using LittenEndian = Reader<std::endian::little>;
using BigEndian = Reader<std::endian::big>;

} // namespace endian

} // namespace bela

#endif
