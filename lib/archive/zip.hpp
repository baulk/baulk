///
#ifndef BAULK_ZIP_HPP
#define BAULK_ZIP_HPP
#include <cstdint>
#include <bela/base.hpp>
// https://www.winzip.com/win/en/comp_info.html
#ifndef MZ_ZIP_H
typedef struct mz_zip_file_s {
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

} mz_zip_file, mz_zip_entry;
#endif

extern "C" {
int32_t mz_zip_reader_get_raw(void *handle, uint8_t *raw);
}

typedef int32_t (*mz_zip_reader_progress_cb)(void *handle, void *userdata, mz_zip_file *file_info, int64_t position);
typedef int32_t (*mz_zip_reader_entry_cb)(void *handle, void *userdata, mz_zip_file *file_info, const char *path);

namespace baulk::archive::zip {
struct zip_closure {
  void *userdata;
  mz_zip_reader_progress_cb progress;
  mz_zip_reader_entry_cb entry;
};

bool ZipExtract(std::wstring_view file, std::wstring_view dest, bela::error_code &ec,
                const zip_closure *closure = nullptr, int encoding = 0);

} // namespace baulk::archive::zip

#endif