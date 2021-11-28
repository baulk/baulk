///
#ifndef BAULK_ARCHIVE_CRC32_HPP
#define BAULK_ARCHIVE_CRC32_HPP
#include <cstdint>

namespace baulk::archive {
class Summator {
public:
  Summator(uint32_t val = 0) {}
  Summator(const Summator &) = delete;
  Summator &operator=(const Summator &) = delete;
  uint32_t Update(const void *data, size_t bytes);

private:
  uint32_t crc32_target_val{0};
  uint32_t current{0};
};

} // namespace baulk::archive

#endif