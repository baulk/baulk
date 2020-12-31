//
#ifndef BELA_OS_HPP
#define BELA_OS_HPP
#include <cstdint>
#include <type_traits>

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
};
[[nodiscard]] constexpr FileMode operator&(FileMode L, FileMode R) noexcept {
  using I = std::underlying_type_t<FileMode>;
  return static_cast<FileMode>(static_cast<I>(L) & static_cast<I>(R));
}
[[nodiscard]] constexpr FileMode operator|(FileMode L, FileMode R) noexcept {
  using I = std::underlying_type_t<FileMode>;
  return static_cast<FileMode>(static_cast<I>(L) | static_cast<I>(R));
}

} // namespace bela::os

#endif