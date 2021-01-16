//
#ifndef BAULK_ARCHIVE_HPP
#define BAULK_ARCHIVE_HPP
#include <cstdint>
#include <type_traits>
#include <span>
#include <memory>
#include <memory_resource>
#include <bela/os.hpp>
#include <bela/base.hpp>
#include <bela/time.hpp>

namespace baulk::archive {
using bela::ErrCanceled;
using bela::ErrEnded;
using bela::ErrGeneral;
namespace archive_internal {
#ifdef PARALLEL_UNZIP
extern std::pmr::synchronized_pool_resource pool;
#else
extern std::pmr::unsynchronized_pool_resource pool;
#endif
template <typename T> T *Allocate(size_t n = 1) { return reinterpret_cast<T *>(pool.allocate(sizeof(T) * n)); }
template <typename T> void Deallocate(T *data, size_t n = 1) { pool.deallocate(data, sizeof(T) * n); }
} // namespace archive_internal

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
  [[nodiscard]] size_t pos() const { return pos_; }
  [[nodiscard]] size_t &pos() { return pos_; }
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
  size_t pos_{0};
};

class FD {
private:
  void Free() {
    if (fd != INVALID_HANDLE_VALUE) {
      CloseHandle(fd);
      fd = INVALID_HANDLE_VALUE;
    }
  }
  void MoveFrom(FD &&o) {
    Free();
    fd = o.fd;
    o.fd = INVALID_HANDLE_VALUE;
  }

public:
  FD() = default;
  FD(HANDLE fd_) : fd(fd_) {}
  FD(const FD &) = delete;
  FD &operator=(const FD &) = delete;
  FD(FD &&o) noexcept { MoveFrom(std::move(o)); }
  FD &operator=(FD &&o) noexcept {
    MoveFrom(std::move(o));
    return *this;
  }
  ~FD() { Free(); }
  bool SetTime(bela::Time t, bela::error_code &ec);
  // Discard file changes
  bool Discard();
  bool Write(const void *data, size_t bytes, bela::error_code &ec);

private:
  HANDLE fd{INVALID_HANDLE_VALUE};
};
std::optional<std::wstring> PathCat(std::wstring_view root, std::string_view sub);

std::optional<FD> NewFD(std::wstring_view path, bela::error_code &ec, bool overwrite = false);
bool NewSymlink(std::wstring_view path, std::wstring_view linkname, bela::error_code &ec, bool overwrite = false);
} // namespace baulk::archive

#endif