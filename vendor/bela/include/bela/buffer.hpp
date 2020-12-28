///
#ifndef BELA_BUFFER_HPP
#define BELA_BUFFER_HPP
#include <memory>
#include <optional>
#include <span>

namespace bela {
constexpr inline size_t align_length(size_t n) { return (n / 8 + 1) * 8; }
namespace buffer {
template <typename T = uint8_t, typename Allocator = std::allocator<T>> class Buffer {
private:
  typedef Allocator allocator_type;
  allocator_type get_allocator() const noexcept { return alloc_; }
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
      get_allocator().deallocate(data_, capacity_);
      data_ = nullptr;
      capacity_ = 0;
    }
  }
  allocator_type alloc_;

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
    auto b = get_allocator().allocate(n);
    if (size_ != 0) {
      memcpy(b, data_, sizeof(T) * n);
    }
    if (data_ != nullptr) {
      get_allocator().deallocate(data_, capacity_);
    }
    data_ = b;
    capacity_ = n;
  }
  template <typename I> [[nodiscard]] const I *cast() const { return reinterpret_cast<const I *>(data_); }
  [[nodiscard]] const T *data() const { return data_; }
  [[nodiscard]] T operator[](const size_t _Off) const noexcept { return *(data_ + _Off); }
  [[nodiscard]] T *data() { return data_; }
  std::span<T> Span() const { return std::span(data_, data_ + size_); }
  // std::span<T> Span
private:
  T *data_{nullptr};
  size_t size_{0};
  size_t capacity_{0};
};
} // namespace buffer
using Buffer = bela::buffer::Buffer<uint8_t, std::allocator<uint8_t>>;
} // namespace bela

#endif