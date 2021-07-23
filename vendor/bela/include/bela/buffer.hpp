///
#ifndef BELA_BUFFER_HPP
#define BELA_BUFFER_HPP
#include <memory>
#include <optional>
#include <span>
#include <string>
#include "bytes_view.hpp"

namespace bela {

class Buffer {
private:
  std::allocator<uint8_t> alloc_;
  void MoveFrom(Buffer &&other) {
    Free(); // Free self
    data_ = other.data_;
    alloc_ = std::move(other.alloc_);
    other.data_ = nullptr;
    capacity_ = other.capacity_;
    other.capacity_ = 0;
    size_ = other.size_;
    other.size_ = 0;
  }
  void Free() {
    if (data_ != nullptr) {
      alloc_.deallocate(data_, capacity_);
      data_ = nullptr;
      capacity_ = 0;
      size_ = 0;
    }
  }

public:
  Buffer() = default;
  Buffer(size_t maxsize) { grow(maxsize); }
  Buffer(Buffer &&other) { MoveFrom(std::move(other)); }
  Buffer &operator=(Buffer &&other) {
    MoveFrom(std::move(other));
    return *this;
  }
  Buffer(const Buffer &) = delete;
  Buffer &operator=(const Buffer &) = delete;
  ~Buffer() { Free(); }
  [[nodiscard]] size_t size() const { return size_; }
  [[nodiscard]] size_t &size() { return size_; }
  [[nodiscard]] size_t capacity() const { return capacity_; }
  void grow(size_t n) {
    if (n <= capacity_) {
      return;
    }
    auto b = alloc_.allocate(n);
    if (size_ != 0) {
      memcpy(b, data_, size_);
    }
    if (data_ != nullptr) {
      alloc_.deallocate(data_, capacity_);
    }
    data_ = b;
    capacity_ = n;
  }
  [[nodiscard]] const uint8_t *data() const { return data_; }
  [[nodiscard]] uint8_t operator[](const size_t _Off) const noexcept { return *(data_ + _Off); }
  [[nodiscard]] uint8_t *data() { return data_; }
  std::span<const uint8_t> make_const_span() const { return std::span{data_, size_}; }
  std::span<uint8_t> make_span(size_t spanlen = std::string_view::npos) const {
    return std::span{data_, (std::min)(capacity_, spanlen)};
  }
  auto as_bytes_view() const { return bytes_view(data_, size_); }

private:
  uint8_t *data_{nullptr};
  size_t size_{0};
  size_t capacity_{0};
};

} // namespace bela

#endif