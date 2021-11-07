///
#include "../lzmasdk/C/7z.h"
#include <baulk/allocate.hpp>

namespace baulk::archive::sevenzip {
namespace {
void *SzAlloc(ISzAllocPtr p, size_t size) {
  (void)p;
  return mem::allocate(size);
}
void SzFree(ISzAllocPtr p, void *address) {
  (void)p;
  mem::deallocate(address);
}
} // namespace

} // namespace baulk::archive::sevenzip