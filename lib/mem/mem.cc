//
#include <baulk/allocate.hpp>
#include "mimalloc/include/mimalloc.h"

namespace baulk::mem {
void *allocate(size_t bytes) noexcept { return mi_malloc(bytes); }
void deallocate(void *p) noexcept { mi_free(p); }

void Buffer::MoveFrom(Buffer &&other) {
  Free(); // Free self
  data_ = other.data_;
  other.data_ = nullptr;
  capacity_ = other.capacity_;
  other.capacity_ = 0;
  size_ = other.size_;
  other.size_ = 0;
}

void Buffer::Free() {
  if (data_ == nullptr) {
    return;
  }
  mi_free(data_);
  data_ = nullptr;
  capacity_ = 0;
}

void Buffer::grow(size_t new_capacity) {
  if (new_capacity <= capacity_) {
    return;
  }
  auto b = reinterpret_cast<uint8_t *>(mi_malloc(new_capacity));
  if (size_ != 0) {
    memcpy(b, data_, size_);
  }
  if (data_ != nullptr) {
    mi_free(data_);
  }
  data_ = b;
  capacity_ = new_capacity;
}
} // namespace baulk::mem