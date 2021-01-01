///
#include "zipinternal.hpp"
#include <lzma.h>

namespace baulk::archive::zip {
enum header_state { INCOMPLETE, OUTPUT, DONE };

#define HEADER_BYTES_ZIP 9
#define HEADER_MAGIC_LENGTH 4
#define HEADER_MAGIC1_OFFSET 0
#define HEADER_MAGIC2_OFFSET 2
#define HEADER_SIZE_OFFSET 9
#define HEADER_SIZE_LENGTH 8
#define HEADER_PARAMETERS_LENGTH 5
#define HEADER_LZMA_ALONE_LENGTH (HEADER_PARAMETERS_LENGTH + HEADER_SIZE_LENGTH)

constexpr uint64_t maximum_compressed_size(uint64_t uncompressed_size) {
  /*
   According to https://sourceforge.net/p/sevenzip/discussion/45797/thread/b6bd62f8/
   1) you can use
   outSize = 1.10 * originalSize + 64 KB.
   in most cases outSize is less then 1.02 from originalSize.
   2) You can try LZMA2, where
   outSize can be = 1.001 * originalSize + 1 KB.
   */
  /* 13 bytes added for lzma alone header */
  auto compressed_size = static_cast<uint64_t>(static_cast<double>(uncompressed_size) * 1.1) + 64 * 1024 + 13;
  if (compressed_size < uncompressed_size) {
    return UINT64_MAX;
  }
  return compressed_size;
}

// XZ
bool Reader::decompressXz(const File &file, const Receiver &receiver, int64_t &decompressed,
                          bela::error_code &ec) const {
  lzma_stream zs;
  memset(&zs, 0, sizeof(zs));
  auto ret = lzma_stream_decoder(&zs, UINT64_MAX, LZMA_CONCATENATED);
  if (ret != LZMA_OK) {
    ec = bela::make_error_code(ret, L"lzma_stream_decoder error ", ret);
    return false;
  }
  Buffer out(outsize);
  Buffer in(insize);
  auto csize = file.compressedSize;
  lzma_action action = LZMA_RUN; // no C26812
  zs.next_out = out.data();
  zs.avail_out = outsize;
  uint32_t crc32val = 0;
  while (csize != 0) {
    auto minsize = (std::min)(csize, static_cast<uint64_t>(insize));
    if (!ReadFull(in.data(), static_cast<size_t>(minsize), ec)) {
      return false;
    }
    zs.next_in = in.data();
    zs.avail_in = minsize;
    if (csize == minsize) {
      action = LZMA_FINISH;
    }
    ret = lzma_code(&zs, action);
    if (zs.avail_out == 0 || ret != LZMA_OK) {
      auto have = minsize - zs.avail_out;
      crc32val = crc32_fast(out.data(), have, crc32val);
      if (!receiver(out.data(), have)) {
        ec = bela::make_error_code(ErrCanceled, L"canceled");
        return false;
      }
      decompressed += have;
      zs.next_out = out.data();
      zs.avail_out = outsize;
    }
    csize -= minsize;
    if (ret == LZMA_STREAM_END) {
      break;
    }
    if (ret != LZMA_OK) {
      switch (ret) {
      case LZMA_MEM_ERROR:
        ec = bela::make_error_code(L"memory error");
        return false;
      case LZMA_FORMAT_ERROR:
        ec = bela::make_error_code(L"File format not recognized");
        return false;
      case LZMA_OPTIONS_ERROR:
        ec = bela::make_error_code(L"Unsupported compression options");
        return false;
      case LZMA_DATA_ERROR:
        ec = bela::make_error_code(L"File is corrupt");
        return false;
      case LZMA_BUF_ERROR:
        ec = bela::make_error_code(L"Unexpected end of input");
        return false;
      default:
        ec = bela::make_error_code(L"Internal error (bug)");
        return false;
      }
    }
  }
  if (crc32val != file.crc32) {
    ec = bela::make_error_code(1, L"crc32 want ", file.crc32, L" got ", crc32val, L" not match");
    return false;
  }
  return true;
}
// LZMA2
bool Reader::decompressLZMA2(const File &file, const Receiver &receiver, int64_t &decompressed,
                             bela::error_code &ec) const {
  lzma_stream zstr;
  memset(&zstr, 0, sizeof(zstr));
  return false;
}

} // namespace baulk::archive::zip