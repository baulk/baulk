///
#ifndef BAULK_ZIP_HPP
#define BAULK_ZIP_HPP
#include <cstdint>
#include <bela/base.hpp>

namespace zip {
[[maybe_unused]] constexpr auto BAULK_Z_DEFLATED = 8;
[[maybe_unused]] constexpr auto BAULK_Z_BZIP2ED = 12;
#pragma pack(2)
// no include magic
struct zip_file_info64_t {
  uint16_t version;
  uint16_t version_needed;
  uint16_t flag;
  uint16_t compression_method;
  uint32_t dosdate;
  uint32_t crc;
  uint64_t compressed_size;
  uint64_t uncompressed_size;
  uint16_t size_filename;
  uint16_t size_fileextra;
  uint16_t size_filecomment;

  uint16_t disk_num_start;
  uint16_t internal_fa;
  uint32_t external_fa;
};
// n file name
// extra field

struct zip_file_info_t {
  uint16_t version;
  uint16_t version_needed;
  uint16_t flag;
  uint16_t compression_method;
  uint32_t dosdate;
  uint32_t crc;
  uint32_t compressed_size;
  uint32_t uncompressed_size;
  uint16_t size_filename;
  uint16_t size_fileextra;
  uint16_t size_filecomment;

  uint16_t disk_num_start;
  uint16_t internal_fa;
  uint32_t external_fa;
};

#pragma pack()

class ZipReader {
public:
  ZipReader() = default;
  ZipReader(const ZipReader &) = delete;
  ZipReader &operator=(const ZipReader &) = delete;
  bool Initialize(std::wstring_view zipfile, std::wstring_view dest,
                  bela::error_code &ec);

private:
  std::wstring destination;
  HANDLE FileHandle{INVALID_HANDLE_VALUE};
  int64_t filesize{0};
};

} // namespace zip

#endif