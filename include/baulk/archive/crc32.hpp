///
#ifndef BAULK_ARCHIVE_CRC32_HPP
#define BAULK_ARCHIVE_CRC32_HPP
#include <cstdint>
#include "details/crc32.h"

namespace baulk::archive {
class Summator {
public:
  Summator(uint32_t val = 0) : crc32_target_val(val) {}
  void Update(const void *data, size_t bytes) {
    if (crc32_target_val == 0) {
      return;
    }
    current = crc32_fast(data, bytes, current);
  }
  bool Valid() const {
    if (crc32_target_val == 0) {
      return true;
    }
    return current == crc32_target_val;
  }
  const auto Current() const { return current; }

private:
  uint32_t crc32_target_val{0};
  uint32_t current{0};
};

} // namespace baulk::archive

#endif