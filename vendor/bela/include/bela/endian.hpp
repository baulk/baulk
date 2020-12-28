// endian.hpp
#ifndef BELA_ENDIAN_HPP
#define BELA_ENDIAN_HPP
#include <cstdint>
#include <cstring>
#include <type_traits>
#include <bit>

namespace bela {
constexpr inline bool IsBigEndian() { return std::endian::native == std::endian::big; }
constexpr inline bool IsLittleEndian() { return std::endian::native == std::endian::little; }

inline uint16_t swap16(uint16_t value) {
#if defined(_MSC_VER)
  // The DLL version of the runtime lacks these functions (bug!?), but in a
  // release build they're replaced with BSWAP instructions anyway.
  return _byteswap_ushort(value);
#else
  uint16_t Hi = value << 8;
  uint16_t Lo = value >> 8;
  return Hi | Lo;
#endif
}
// We use C++17. so GCC version must > 8.0. __builtin_bswap32 awayls exists
inline uint32_t swap32(uint32_t value) {
#if defined(__llvm__) || (defined(__GNUC__) && !defined(__ICC))
  return __builtin_bswap32(value);
#elif defined(_MSC_VER)
  return _byteswap_ulong(value);
#else
  uint32_t Byte0 = value & 0x000000FF;
  uint32_t Byte1 = value & 0x0000FF00;
  uint32_t Byte2 = value & 0x00FF0000;
  uint32_t Byte3 = value & 0xFF000000;
  return (Byte0 << 24) | (Byte1 << 8) | (Byte2 >> 8) | (Byte3 >> 24);
#endif
}

inline uint64_t swap64(uint64_t value) {
#if defined(__llvm__) || (defined(__GNUC__) && !defined(__ICC))
  return __builtin_bswap64(value);
#elif defined(_MSC_VER)
  return _byteswap_uint64(value);
#else
  uint64_t Hi = swap32(uint32_t(value));
  uint32_t Lo = swap32(uint32_t(value >> 32));
  return (Hi << 32) | Lo;
#endif
}

inline unsigned char bswap(unsigned char v) { return v; }
inline signed char bswap(signed char v) { return v; }
inline unsigned short bswap(unsigned short v) { return swap16(v); }
inline signed short bswap(signed short v) { return static_cast<signed short>(swap16(static_cast<uint16_t>(v))); }
inline unsigned int bswap(unsigned int v) { return static_cast<unsigned int>(swap32(static_cast<uint32_t>(v))); }
inline signed int bswap(signed int v) { return static_cast<signed int>(swap32(static_cast<uint32_t>(v))); }

inline unsigned long bswap(unsigned long v) {
  static_assert(sizeof(unsigned long) == 8 || sizeof(unsigned long) == 4, "unsigned long size must 8 byte or 4 byte");
  if constexpr (sizeof(unsigned long) == 8) {
    return static_cast<unsigned long>(swap64(static_cast<uint64_t>(v)));
  }
  return static_cast<unsigned long>(swap32(static_cast<uint32_t>(v)));
}
inline signed long bswap(signed long v) {
  static_assert(sizeof(signed long) == 8 || sizeof(signed long) == 4, "signed long size must 8 byte or 4 byte");
  if constexpr (sizeof(signed long) == 8) {
    return static_cast<signed long>(swap64(static_cast<uint64_t>(v)));
  }
  return static_cast<signed long>(swap32(static_cast<uint32_t>(v)));
}

inline unsigned long long bswap(unsigned long long v) {
  return static_cast<unsigned long long>(swap64(static_cast<uint64_t>(v)));
}

inline signed long long bswap(signed long long v) {
  return static_cast<signed long long>(swap64(static_cast<uint64_t>(v)));
}

// SO Network order is BigEndian
template <typename T> inline T htons(T v) {
  static_assert(std::is_integral_v<T>, "htnos requires integer");
  if constexpr (IsBigEndian()) {
    return v;
  }
  return bswap(v);
}
template <typename T> inline T ntohs(T v) {
  static_assert(std::is_integral_v<T>, "ntohs requires integer");
  if constexpr (IsBigEndian()) {
    return v;
  }
  return bswap(v);
}

template <typename T> inline T fromle(T i) {
  static_assert(std::is_integral<T>::value, "fromle requires integer");
  if constexpr (IsBigEndian()) {
    return bswap(i);
  }
  return i;
}

template <typename T> inline T frombe(T i) {
  static_assert(std::is_integral<T>::value, "frombe requires integer");
  if constexpr (IsBigEndian()) {
    return i;
  }
  return bswap(i);
}

template <typename T> inline T unaligned_load(const void *p) {
  static_assert(std::is_integral_v<T>, "unaligned_load requires integer");
  T t;
  memcpy(&t, p, sizeof(T));
  return t;
}

template <typename T> inline T cast_fromle(const void *p) {
  static_assert(std::is_integral_v<T>, "cast_fromle requires integer");
  auto v = unaligned_load<T>(p);
  if constexpr (IsLittleEndian()) {
    return v;
  }
  return bswap(v);
}

template <typename T> inline T cast_frombe(const void *p) {
  static_assert(std::is_integral_v<T>, "cast_frombe requires integer");
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
  void Reset(const void *p, size_t len) {
    data = reinterpret_cast<const uint8_t *>(p);
    size = len;
  }
  template <typename T> T Read() {
    static_assert(std::is_integral_v<T>, "bela::endian::Reader::Read requires integer");
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
  template <typename T = char> const T *Data() const { return reinterpret_cast<const T *>(data); }

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
