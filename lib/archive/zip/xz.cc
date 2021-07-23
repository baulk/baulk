///
#include "zipinternal.hpp"
#include <lzma.h>

namespace baulk::archive::zip {
constexpr size_t xzoutsize = 256 * 1024;
constexpr size_t xzinsize = 128 * 1024;
// XZ
bool Reader::decompressXz(const File &file, const Writer &w, bela::error_code &ec) const {
  lzma_stream zs = LZMA_STREAM_INIT;
  auto ret = lzma_stream_decoder(&zs, UINT64_MAX, LZMA_CONCATENATED);
  if (ret != LZMA_OK) {
    ec = bela::make_error_code(ret, L"lzma_stream_decoder error ", ret);
    return false;
  }
  Buffer out(xzoutsize);
  Buffer in(xzinsize);
  auto csize = file.compressedSize;
  lzma_action action = LZMA_RUN; // no C26812
  zs.next_in = nullptr;
  zs.avail_in = 0;
  zs.next_out = out.data();
  zs.avail_out = xzoutsize;
  uint32_t crc32val = 0;
  for (;;) {
    if (zs.avail_in == 0 && csize != 0) {
      auto minsize = (std::min)(csize, static_cast<uint64_t>(xzinsize));
      if (!fd.ReadFull({in.data(), static_cast<size_t>(minsize)}, ec)) {
        return false;
      }
      zs.next_in = in.data();
      zs.avail_in = minsize;
      csize -= minsize;
      if (csize == 0) {
        action = LZMA_FINISH;
      }
    }
    ret = lzma_code(&zs, action);
    if (zs.avail_out == 0 || ret == LZMA_STREAM_END) {
      auto have = xzoutsize - zs.avail_out;
      crc32val = crc32_fast(out.data(), have, crc32val);
      if (!w(out.data(), have)) {
        ec = bela::make_error_code(ErrCanceled, L"canceled");
        return false;
      }
      zs.next_out = out.data();
      zs.avail_out = xzoutsize;
    }
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
  if (crc32val != file.crc32sum) {
    ec = bela::make_error_code(ErrGeneral, L"crc32 want ", file.crc32sum, L" got ", crc32val, L" not match");
    return false;
  }
  return true;
}

#pragma pack(push)
#pragma pack(1)
struct alone_header {
  uint8_t bytes[5]; // lzma_params_length
  uint64_t uncompressed_size;
};
#pragma pack(pop)

// LZMA
bool Reader::decompressLZMA(const File &file, const Writer &w, bela::error_code &ec) const {
  lzma_stream zs;
  memset(&zs, 0, sizeof(zs));
  if (auto ret = lzma_alone_decoder(&zs, UINT64_MAX); ret != LZMA_OK) {
    ec = bela::make_error_code(ret, L"lzma_stream_decoder error ", ret);
    return false;
  }
  auto closer = bela::finally([&] { lzma_end(&zs); });
  // cat /bin/ls | lzma | xxd | head -n 1
  // $ cat stream_inside_zipx | xxd | head -n 1
  // 00000000: 0914 0500 5d00 8000 0000 2814 .... ....
  uint8_t d[16] = {0};
  if (!fd.ReadFull({d, 9}, ec)) {
    return false;
  }
  if (d[2] != 0x05 || d[3] != 0x00) {
    ec = bela::make_error_code(ErrGeneral, L"Invalid LZMA data");
    return false;
  }
  alone_header ah{0};
  memcpy(ah.bytes, d + 4, 5);
  ah.uncompressed_size = UINT64_MAX;
  Buffer out(xzoutsize);
  Buffer in(xzinsize);
  zs.next_in = reinterpret_cast<const uint8_t *>(&ah);
  zs.avail_in = sizeof(ah);
  zs.total_in = 0;
  zs.next_out = out.data();
  zs.avail_out = xzoutsize;
  zs.total_out = 0;
  int ret = LZMA_OK;
  if (ret = lzma_code(&zs, LZMA_RUN); ret != LZMA_OK) {
    ec = bela::make_error_code(L"LZMA stream initialization error");
    return false;
  }
  uint32_t crc32val = 0;
  auto csize = file.compressedSize - 9;
  lzma_action action = LZMA_RUN;
  for (;;) {
    if (zs.avail_in == 0 && csize > 0) {
      auto minsize = (std::min)(csize, static_cast<uint64_t>(xzinsize));
      if (!fd.ReadFull({in.data(), static_cast<size_t>(minsize)}, ec)) {
        return false;
      }
      zs.next_in = in.data();
      zs.avail_in = minsize;
      csize -= minsize;
      if (csize == 0) {
        action = LZMA_FINISH;
      }
    }
    ret = lzma_code(&zs, action);
    if (zs.avail_out == 0 || ret == LZMA_STREAM_END) {
      auto have = xzoutsize - zs.avail_out;
      crc32val = crc32_fast(out.data(), have, crc32val);
      if (!w(out.data(), have)) {
        ec = bela::make_error_code(ErrCanceled, L"canceled");
        return false;
      }
      zs.next_out = out.data();
      zs.avail_out = xzoutsize;
    }
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
  if (crc32val != file.crc32sum) {
    ec = bela::make_error_code(ErrGeneral, L"crc32 want ", file.crc32sum, L" got ", crc32val, L" not match");
    return false;
  }
  return true;
}

} // namespace baulk::archive::zip