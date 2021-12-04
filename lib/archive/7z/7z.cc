///
#include "../lzmasdk/C/7z.h"
#include <baulk/allocate.hpp>

namespace baulk::archive::n7z {
namespace {
void *SzAlloc(ISzAllocPtr p, size_t size) {
  (void)p;
  return mi_malloc(size);
}
void SzFree(ISzAllocPtr p, void *address) {
  (void)p;
  mi_free(address);
}
} // namespace

} // namespace baulk::archive::n7z