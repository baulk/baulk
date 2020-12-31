//
#ifndef BAULK_ARCHIVE_HPP
#define BAULK_ARCHIVE_HPP
#include <cstdint>
#include <type_traits>
#include <span>

namespace baulk::archive {
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

class Buffer {
private:
  void MoveFrom(Buffer &&other) {
    Free(); // Free self
    data_ = other.data_;
    other.data_ = nullptr;
    capacity_ = other.capacity_;
    other.capacity_ = 0;
    size_ = other.size_;
    other.size_ = 0;
  }
  void Free();

public:
  Buffer() = default;
  Buffer(size_t maxsize) { grow(maxsize); }
  Buffer(Buffer &&other) noexcept { MoveFrom(std::move(other)); }
  Buffer &operator=(Buffer &&other) noexcept {
    MoveFrom(std::move(other));
    return *this;
  }
  Buffer(const Buffer &) = delete;
  Buffer &operator=(const Buffer &) = delete;
  ~Buffer() { Free(); }
  [[nodiscard]] size_t size() const { return size_; }
  [[nodiscard]] size_t &size() { return size_; }
  [[nodiscard]] size_t capacity() const { return capacity_; }
  void grow(size_t n);
  template <typename I> [[nodiscard]] const I *cast() const { return reinterpret_cast<const I *>(data_); }
  [[nodiscard]] const uint8_t *data() const { return data_; }
  [[nodiscard]] uint8_t operator[](const size_t _Off) const noexcept { return *(data_ + _Off); }
  [[nodiscard]] uint8_t *data() { return data_; }
  auto Span() const { return std::span(data_, data_ + size_); }

private:
  uint8_t *data_{nullptr};
  size_t size_{0};
  size_t capacity_{0};
};

} // namespace baulk::archive

#endif