//
#ifndef BAULK_ALLOCATE_HPP
#define BAULK_ALLOCATE_HPP
#include <cstdint>
#include <cstring>
#include <type_traits>
#include <span>
#include <memory>
#include <memory_resource>
#include <bela/os.hpp>
#include <bela/base.hpp>
#include <bela/time.hpp>
#include <bela/bytes_view.hpp>

extern "C" {
[[nodiscard]] void *mi_malloc(size_t size) noexcept;
void mi_free(void *p) noexcept;
}

namespace baulk::mem {
template <typename T> T *allocate(size_t n = 1) noexcept { return reinterpret_cast<T *>(mi_malloc(sizeof(T) * n)); }
inline void deallocate(void *p) noexcept { mi_free(p); }
inline void *allocate_simple(void *opaque, size_t size) noexcept {
  (void)opaque;
  return mi_malloc(size);
}
inline void deallocate_simple(void *opaque, void *address) noexcept {
  (void)opaque;
  mi_free(address);
}
inline void *allocate_bz(void *opaque, int items, int size) noexcept {
  (void)opaque;
  return mi_malloc(static_cast<size_t>(items) * size);
}
inline void *allocate_zlib(void *opaque, uint32_t items, uint32_t size) noexcept {
  (void)opaque;
  return mi_malloc(static_cast<size_t>(items) * size);
}

inline void *allocate_xz(void *opaque, size_t nmemb, size_t size) noexcept {
  (void)opaque;
  return mi_malloc(static_cast<size_t>(nmemb) * size);
}
// Buffer use mimalloc
class Buffer {
private:
  void MoveFrom(Buffer &&other);
  void Free();

public:
  Buffer() = default;
  Buffer(size_t new_capacity) { grow(new_capacity); }
  Buffer(Buffer &&other) noexcept { MoveFrom(std::move(other)); }
  Buffer &operator=(Buffer &&other) noexcept {
    MoveFrom(std::move(other));
    return *this;
  }
  Buffer(const Buffer &) = delete;
  Buffer &operator=(const Buffer &) = delete;
  ~Buffer() { Free(); }
  void grow(size_t new_capacity);
  [[nodiscard]] size_t &size() { return size_; }
  [[nodiscard]] size_t &pos() { return pos_; }
  [[nodiscard]] uint8_t *data() { return data_; }

  // const function
  [[nodiscard]] size_t size() const { return size_; }
  [[nodiscard]] size_t capacity() const { return capacity_; }
  [[nodiscard]] size_t pos() const { return pos_; }
  template <typename I>
  requires bela::standard_layout<I>
  [[nodiscard]] const I *unchecked_cast() const { return reinterpret_cast<const I *>(data_); }
  [[nodiscard]] const uint8_t *data() const { return data_; }
  [[nodiscard]] uint8_t operator[](const size_t _Off) const noexcept { return *(data_ + _Off); }
  auto make_span() const { return std::span(data_, data_ + size_); }
  auto as_bytes_view() const { return bela::bytes_view(data_, size_); }

private:
  uint8_t *data_{nullptr};
  size_t size_{0};
  size_t capacity_{0};
  size_t pos_{0};
};

} // namespace baulk::mem

#endif