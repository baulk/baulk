///
#ifndef BAULK_ZIP_HPP
#define BAULK_ZIP_HPP
#include <cstdint>
#include <bela/base.hpp>

namespace baulk::archive::zip {
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

struct baulk_zip_file {
  uint16_t version_madeby;     /* version made by */
  uint16_t version_needed;     /* version needed to extract */
  uint16_t flag;               /* general purpose bit flag */
  uint16_t compression_method; /* compression method */
  time_t modified_date;        /* last modified date in unix time */
  time_t accessed_date;        /* last accessed date in unix time */
  time_t creation_date;        /* creation date in unix time */
  uint32_t crc;                /* crc-32 */
  int64_t compressed_size;     /* compressed size */
  int64_t uncompressed_size;   /* uncompressed size */
  uint16_t filename_size;      /* filename length */
  uint16_t extrafield_size;    /* extra field length */
  uint16_t comment_size;       /* file comment length */
  uint32_t disk_number;        /* disk number start */
  int64_t disk_offset;         /* relative offset of local header */
  uint16_t internal_fa;        /* internal file attributes */
  uint32_t external_fa;        /* external file attributes */

  const char *filename;      /* filename utf8 null-terminated string */
  const uint8_t *extrafield; /* extrafield data */
  const char *comment;       /* comment utf8 null-terminated string */
  const char *linkname;      /* sym-link filename utf8 null-terminated string */

  uint16_t zip64;              /* zip64 extension mode */
  uint16_t aes_version;        /* winzip aes extension if not 0 */
  uint8_t aes_encryption_mode; /* winzip aes encryption mode */
};

class ZipReader {
public:
  ZipReader() = default;
  ZipReader(const ZipReader &) = delete;
  ZipReader &operator=(const ZipReader &) = delete;
  ~ZipReader();
  bool Initialize(std::wstring_view zipfile, std::wstring_view dest,
                  bela::error_code &ec);
  size_t Decompressed() const { return decompressed; }

private:
  std::wstring destination;
  HANDLE FileHandle{INVALID_HANDLE_VALUE};
  int64_t filesize{0};
  size_t decompressed{0};
};

} // namespace zip

#endif