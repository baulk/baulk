//
#include "tarinternal.hpp"
#include <charconv>

namespace baulk::archive::tar {

inline std::string cleanupName(const void *data, size_t N) {
  auto p = reinterpret_cast<const char *>(data);
  auto pos = memchr(p, 0, N);
  if (pos == nullptr) {
    return std::string(p, N);
  }
  N = reinterpret_cast<const char *>(pos) - p;
  return std::string(p, N);
}

template <size_t N> std::string cleanupName(char (&aArr)[N]) { return cleanupName(aArr, N); }

inline int64_t parseNumeric8(const char *p, size_t char_cnt) {
  int64_t val = 0;
  auto res = std::from_chars(p, p + char_cnt, val, 8);
  return val;
}

inline int64_t parseNumeric10(const char *p, size_t char_cnt) {
  int64_t val = 0;
  auto res = std::from_chars(p, p + char_cnt, val, 10);
  return val;
}

/*
 * Parse a base-256 integer.  This is just a variable-length
 * twos-complement signed binary value in big-endian order, except
 * that the high-order bit is ignored.  The values here can be up to
 * 12 bytes, so we need to be careful about overflowing 64-bit
 * (8-byte) integers.
 *
 * This code unashamedly assumes that the local machine uses 8-bit
 * bytes and twos-complement arithmetic.
 */
static int64_t parseNumeric256(const char *_p, size_t char_cnt) {
  uint64_t l;
  auto *p = reinterpret_cast<const unsigned char *>(_p);
  unsigned char c{0}, neg{0};

  /* Extend 7-bit 2s-comp to 8-bit 2s-comp, decide sign. */
  c = *p;
  if (c & 0x40) {
    neg = 0xff;
    c |= 0x80;
    l = ~0ull;
  } else {
    neg = 0;
    c &= 0x7f;
    l = 0;
  }

  /* If more than 8 bytes, check that we can ignore
   * high-order bits without overflow. */
  while (char_cnt > sizeof(int64_t)) {
    --char_cnt;
    if (c != neg)
      return neg ? INT64_MIN : INT64_MAX;
    c = *++p;
  }

  /* c is first byte that fits; if sign mismatch, return overflow */
  if ((c ^ neg) & 0x80) {
    return neg ? INT64_MIN : INT64_MAX;
  }

  /* Accumulate remaining bytes. */
  while (--char_cnt > 0) {
    l = (l << 8) | c;
    c = *++p;
  }
  l = (l << 8) | c;
  /* Return signed twos-complement value. */
  return (int64_t)(l);
}

/*-
 * Convert text->integer.
 *
 * Traditional tar formats (including POSIX) specify base-8 for
 * all of the standard numeric fields.  This is a significant limitation
 * in practice:
 *   = file size is limited to 8GB
 *   = rdevmajor and rdevminor are limited to 21 bits
 *   = uid/gid are limited to 21 bits
 *
 * There are two workarounds for this:
 *   = pax extended headers, which use variable-length string fields
 *   = GNU tar and STAR both allow either base-8 or base-256 in
 *      most fields.  The high bit is set to indicate base-256.
 *
 * On read, this implementation supports both extensions.
 */
inline int64_t parseNumeric(const char *p, size_t char_cnt) {
  /*
   * Technically, GNU tar considers a field to be in base-256
   * only if the first byte is 0xff or 0x80.
   */
  if ((*p & 0x80) != 0) {
    return (parseNumeric256(p, char_cnt));
  }
  return (parseNumeric8(p, char_cnt));
}

template <size_t N> int64_t parseNumeric(char (&aArr)[N]) { return parseNumeric(aArr, N); }

FileReader::~FileReader() {
  if (fd != INVALID_HANDLE_VALUE && needClosed) {
    CloseHandle(fd);
  }
}
ssize_t FileReader::Read(void *buffer, size_t len, bela::error_code &ec) {
  DWORD drSize = {0};
  if (::ReadFile(fd, buffer, static_cast<DWORD>(len), &drSize, nullptr) != TRUE) {
    ec = bela::make_system_error_code(L"ReadFile: ");
    return -1;
  }
  return static_cast<ssize_t>(drSize);
}
ssize_t FileReader::ReadAt(void *buffer, size_t len, int64_t pos, bela::error_code &ec) {
  if (!PositionAt(pos, ec)) {
    return -1;
  }
  return Read(buffer, len, ec);
}
bool FileReader::PositionAt(int64_t pos, bela::error_code &ec) {
  auto li = *reinterpret_cast<LARGE_INTEGER *>(&pos);
  LARGE_INTEGER oli{0};
  if (SetFilePointerEx(fd, li, &oli, SEEK_SET) != TRUE) {
    ec = bela::make_system_error_code(L"SetFilePointerEx: ");
    return false;
  }
  return true;
}

std::shared_ptr<FileReader> OpenFile(std::wstring_view file, bela::error_code &ec) {
  auto fd = CreateFileW(file.data(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL, nullptr);
  if (fd == INVALID_HANDLE_VALUE) {
    ec = bela::make_system_error_code();
    return nullptr;
  }
  LARGE_INTEGER li;
  if (GetFileSizeEx(fd, &li) != TRUE) {
    ec = bela::make_system_error_code(L"GetFileSizeEx: ");
    return nullptr;
  }
  return std::make_shared<FileReader>(fd, li.QuadPart, true);
}

inline bool IsZero(const ustar_header &th) {
  static ustar_header zeroth = {0};
  return memcmp(&th, &zeroth, 0) == 0;
}

bela::ssize_t Reader::Read(void *buffer, size_t size, bela::error_code &ec) {
  if (r == nullptr) {
    return -1;
  }
  return r->Read(buffer, size, ec);
}

// ReadAtLeast
bool Reader::ReadFull(void *buffer, size_t size, bela::error_code &ec) {
  size_t rbytes = 0;
  auto p = reinterpret_cast<uint8_t *>(buffer);
  while (rbytes < size) {
    auto sz = Read(p + rbytes, size - rbytes, ec);
    if (sz == -1) {
      return false;
    }
    if (sz == 0) {
      ec = bela::make_error_code(bela::ErrEnded, L"End of file");
      return false;
    }
    rbytes += sz;
  }
  return true;
}

bool Reader::discard(int64_t bytes, bela::error_code &ec) {
  constexpr int64_t discardSize = 4096;
  uint8_t discardBuffer[4096];
  while (bytes > 0) {
    auto minsize = (std::min)(bytes, discardSize);
    auto n = Read(discardBuffer, minsize, ec);
    if (n <= 0) {
      return false;
    }
    bytes -= n;
  }
  return true;
}

// blockPadding computes the number of bytes needed to pad offset up to the
// nearest block edge where 0 <= n < blockSize.
inline int64_t blockPadding(int64_t offset) { return -offset & (blockSize - 1); }

bool isHeaderOnlyType(char flag) {
  switch (flag) {
  case TypeLink:
    [[fallthrough]];
  case TypeSymlink:
    [[fallthrough]];
  case TypeChar:
    [[fallthrough]];
  case TypeBlock:
    [[fallthrough]];
  case TypeDir:
    [[fallthrough]];
  case TypeFifo:
    return true;
  default:
    break;
  }
  return false;
}

std::optional<Header> Reader::Next(bela::error_code &ec) {
  // read next entry
  for (;;) {
    if (!discard(remainingSize, ec)) {
      return std::nullopt;
    }
    if (!discard(paddingSize, ec)) {
      return std::nullopt;
    }
    if (!ReadFull(&hdr, sizeof(hdr), ec)) {
      return std::nullopt;
    }
    if (hdr.name[0] != 0 || !IsZero(hdr)) {
      break;
    }
  }
  Header th;
  th.Format = FormatUSTAR | FormatPAX | FormatGNU;
  th.Size = parseNumeric(hdr.size);
  th.Name = cleanupName(hdr.name);
  th.Mode = parseNumeric(hdr.mode);
  th.ModeTime = bela::FromUnix(parseNumeric(hdr.mtime), 0);
  auto checksum = parseNumeric(hdr.chksum);
  auto nameLinked = cleanupName(hdr.linkname);
  if (memcmp(hdr.magic, magicUSTAR, sizeof(magicUSTAR)) == 0 &&
      memcmp(hdr.version, versionUSTAR, sizeof(versionUSTAR)) == 0) {
  }
  if (memcmp(hdr.magic, magicGNU, sizeof(magicGNU)) == 0 && memcmp(hdr.version, versionGNU, sizeof(versionGNU)) == 0) {
    // GNU TAR
  }
  auto nb = th.Size;
  if (isHeaderOnlyType(hdr.typeflag)) {
    nb = 0;
  }
  paddingSize = blockPadding(nb);
  switch (hdr.typeflag) {
  case TypeXHeader:
    [[fallthrough]];
  case TypeXGlobalHeader:
    break;
  case TypeGNULongName:
    [[fallthrough]];
  case TypeGNULongLink: {
    std::string realName;
    realName.resize(th.Size);
    if (!ReadFull(realName.data(), th.Size, ec)) {
      return std::nullopt;
    }
    th.Name = cleanupName(realName.data(), realName.size());
    th.Size = 0;
  } break;
  default:
    break;
  }

  return std::make_optional(std::move(th));
}

} // namespace baulk::archive::tar