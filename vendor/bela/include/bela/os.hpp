//
#ifndef BELA_OS_HPP
#define BELA_OS_HPP
#include <cstdint>
#include <type_traits>
#include "base.hpp"

namespace bela::os {
enum FileMode : uint32_t {
  ModeDir = 2147483648,      // d: is a directory
  ModeAppend = 1073741824,   // a: append-only
  ModeExclusive = 536870912, // l: exclusive use
  ModeTemporary = 268435456, // T: temporary file; Plan 9 only
  ModeSymlink = 134217728,   // L: symbolic link
  ModeDevice = 67108864,     // D: device file
  ModeNamedPipe = 33554432,  // p: named pipe (FIFO)
  ModeSocket = 16777216,     // S: Unix domain socket
  ModeSetuid = 8388608,      // u: setuid
  ModeSetgid = 4194304,      // g: setgid
  ModeCharDevice = 4194304,  // c: Unix character device, when ModeDevice is set
  ModeSticky = 1048576,      // t: sticky
  ModeIrregular = 524288,    // ?: non-regular file; nothing else is known about this file

  // Mask for the type bits. For regular files, none will be set.
  ModeType = ModeDir | ModeSymlink | ModeNamedPipe | ModeSocket | ModeDevice | ModeCharDevice | ModeIrregular,

  ModePerm = 0777, // Unix permission bits
};

template <class I>
requires std::integral<I>
[[nodiscard]] constexpr FileMode
operator<<(const FileMode _Arg, const I _Shift) noexcept { // bitwise LEFT SHIFT, every static_cast is intentional
  using K = std::underlying_type_t<FileMode>;
  return static_cast<FileMode>(static_cast<K>(_Arg) << _Shift);
}

template <class I>
requires std::integral<I>
[[nodiscard]] constexpr FileMode
operator>>(const FileMode _Arg, const I _Shift) noexcept { // bitwise RIGHT SHIFT, every static_cast is intentional
  using K = std::underlying_type_t<FileMode>;
  return static_cast<FileMode>(static_cast<K>(_Arg) >> _Shift);
}

[[nodiscard]] constexpr FileMode operator&(FileMode L, FileMode R) noexcept {
  using K = std::underlying_type_t<FileMode>;
  return static_cast<FileMode>(static_cast<K>(L) & static_cast<K>(R));
}
[[nodiscard]] constexpr FileMode operator|(FileMode L, FileMode R) noexcept {
  using K = std::underlying_type_t<FileMode>;
  return static_cast<FileMode>(static_cast<K>(L) | static_cast<K>(R));
}

[[nodiscard]] constexpr FileMode
operator^(const FileMode _Left, const FileMode _Right) noexcept { // bitwise XOR, every static_cast is intentional
  using K = std::underlying_type_t<FileMode>;
  return static_cast<FileMode>(static_cast<K>(_Left) ^ static_cast<K>(_Right));
}

template <class I>
requires std::integral<I>
constexpr FileMode &operator<<=(FileMode &_Arg, const I _Shift) noexcept { // bitwise LEFT SHIFT
  return _Arg = _Arg << _Shift;
}

template <class I>
requires std::integral<I>
constexpr FileMode &operator>>=(FileMode &_Arg, const I _Shift) noexcept { // bitwise RIGHT SHIFT
  return _Arg = _Arg >> _Shift;
}

constexpr FileMode &operator|=(FileMode &_Left, const FileMode _Right) noexcept { // bitwise OR
  return _Left = _Left | _Right;
}

constexpr FileMode &operator&=(FileMode &_Left, const FileMode _Right) noexcept { // bitwise AND
  return _Left = _Left & _Right;
}

constexpr FileMode &operator^=(FileMode &_Left, const FileMode _Right) noexcept { // bitwise XOR
  return _Left = _Left ^ _Right;
}

constexpr bool IsDir(FileMode m) { return (m & ModeDir) != 0; }
constexpr bool IsRegular(FileMode m) { return (m & ModeType) == 0; }
constexpr FileMode Perm(FileMode m) { return m & ModePerm; }
constexpr FileMode Type(FileMode m) { return m & ModeType; }

} // namespace bela::os

#endif