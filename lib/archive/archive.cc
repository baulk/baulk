///
#include <memory_resource>
#include <archive.hpp>

namespace baulk::archive {
// https://en.cppreference.com/w/cpp/memory/unsynchronized_pool_resource
// https://quuxplusone.github.io/blog/2018/06/05/libcpp-memory-resource/
// https://www.bfilipek.com/2020/06/pmr-hacking.html

#ifdef PARALLEL_UNZIP
std::pmr::synchronized_pool_resource pool_;
#else
std::pmr::unsynchronized_pool_resource pool_;
#endif

void Buffer::Free() {
  if (data_ != nullptr) {
    pool_.deallocate(data_, capacity_);
    data_ = nullptr;
    capacity_ = 0;
  }
}

void Buffer::grow(size_t n) {
  if (n <= capacity_) {
    return;
  }
  auto b = reinterpret_cast<uint8_t *>(pool_.allocate(n));
  if (size_ != 0) {
    memcpy(b, data_, n);
  }
  if (data_ != nullptr) {
    pool_.deallocate(data_, capacity_);
  }
  data_ = b;
  capacity_ = n;
}

} // namespace baulk::archive