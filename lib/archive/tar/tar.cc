//
#include "tarinternal.hpp"
#include <bela/str_cat_narrow.hpp>
#include <charconv>

namespace baulk::archive::tar {

inline std::string parseString(const void *data, size_t N) {
  auto p = reinterpret_cast<const char *>(data);
  auto pos = memchr(p, 0, N);
  if (pos == nullptr) {
    return std::string(p, N);
  }
  N = reinterpret_cast<const char *>(pos) - p;
  return std::string(p, N);
}

template <size_t N> std::string parseString(const char (&aArr)[N]) { return parseString(aArr, N); }

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

template <size_t N> int64_t parseNumeric(const char (&aArr)[N]) { return parseNumeric(aArr, N); }

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

inline bool isZeroBlock(const ustar_header &th) {
  static ustar_header zeroth = {0};
  return memcmp(&th, &zeroth, sizeof(ustar_header)) == 0;
}

inline bool IsChecksumEqual(const ustar_header &hdr) {
  auto p = reinterpret_cast<const uint8_t *>(&hdr);
  auto value = parseNumeric(hdr.chksum);
  int64_t check = 0;
  int64_t scheck = 0;
  int i;
  for (i = 0; i < 148; i++) {
    check += p[i];
    scheck = static_cast<int8_t>(p[i]);
  }
  check += 256;
  scheck += 256;
  i = 156;
  for (; i < 512; i++) {
    check += p[i];
    scheck = static_cast<int8_t>(p[i]);
  }
  return (check == value || scheck == value);
}

inline tar_format_t getFormat(const ustar_header &hdr) {
  if (!IsChecksumEqual(hdr)) {
    return FormatUnknown;
  }
  auto star = reinterpret_cast<const star_header *>(&hdr);
  if (memcmp(hdr.magic, magicUSTAR, sizeof(hdr.magic)) == 0) {
    if (memcmp(star->trailer, trailerSTAR, sizeof(trailerSTAR)) == 0) {
      return FormatSTAR;
    }
    return FormatUSTAR | FormatPAX;
  }
  if (memcmp(hdr.magic, magicGNU, sizeof(magicGNU)) == 0 && memcmp(hdr.version, versionGNU, sizeof(versionGNU)) == 0) {
    return FormatGNU;
  }
  return FormatV7;
}

// blockPadding computes the number of bytes needed to pad offset up to the
// nearest block edge where 0 <= n < blockSize.
inline constexpr int64_t blockPadding(int64_t offset) { return -offset & (blockSize - 1); }

inline constexpr bool isHeaderOnlyType(char flag) {
  return (flag == TypeLink || flag == TypeSymlink || flag == TypeChar || flag == TypeBlock || flag == TypeDir ||
          flag == TypeFifo);
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

bool Reader::readHeader(Header &h, bela::error_code &ec) {
  ustar_header hdr{0};
  if (!ReadFull(&hdr, sizeof(hdr), ec)) {
    return false;
  }
  if (isZeroBlock(hdr)) {
    if (!ReadFull(&hdr, sizeof(hdr), ec)) {
      return false;
    }
    if (isZeroBlock(hdr)) {
      ec = bela::make_error_code(bela::ErrEnded, L"tar stream end");
      return false;
    }
    ec = bela::make_error_code(L"invalid tar header");
    return false;
  }
  if (h.Format = getFormat(hdr); h.Format == FormatUnknown) {
    ec = bela::make_error_code(L"invalid tar header");
    return false;
  }
  h.Typeflag = hdr.typeflag;
  h.Name = parseString(hdr.name);
  h.LinkName = parseString(hdr.linkname);
  h.Size = parseNumeric(hdr.size);
  h.UID = static_cast<int>(parseNumeric(hdr.uid));
  h.GID = static_cast<int>(parseNumeric(hdr.gid));
  h.ModTime = bela::FromUnix(parseNumeric(hdr.mtime), 0);
  if (h.Format > FormatV7) {
    h.Uname = parseString(hdr.uname);
    h.Gname = parseString(hdr.gname);
    h.Devmajor = parseNumeric(hdr.devmajor);
    h.Devminor = parseNumeric(hdr.devminor);
    std::string prefix;
    if ((h.Format & (FormatUSTAR | FormatPAX)) != 0) {
      prefix = parseString(hdr.prefix);
    } else if ((h.Format & FormatSTAR) != 0) {
      auto star = reinterpret_cast<const star_header *>(&hdr);
      prefix = parseString(star->prefix);
      h.AccessTime = bela::FromUnix(parseNumeric(star->atime), 0);
      h.ChangeTime = bela::FromUnix(parseNumeric(star->ctime), 0);
    } else if ((h.Format & FormatGNU) != 0) {
      auto gnu = reinterpret_cast<const gnutar_header *>(&hdr);
      if (gnu->atime[0] != 0) {
        h.AccessTime = bela::FromUnix(parseNumeric(gnu->atime), 0);
      }
      if (gnu->ctime[0] != 0) {
        h.ChangeTime = bela::FromUnix(parseNumeric(gnu->ctime), 0);
      }
    }
    if (!prefix.empty()) {
      h.Name = bela::narrow::StringCat(prefix, "/", h.Name);
    }
  }
  auto nb = h.Size;
  if (isHeaderOnlyType(h.Typeflag)) {
    nb = 0;
  }
  paddingSize = blockPadding(nb);
  return true;
}

bool parsePAXTime(std::string_view p, bela::Time &t, bela::error_code &ec) {
  auto ss = p;
  std::string_view sn;
  if (auto pos = p.find('.'); pos != std::string_view::npos) {
    ss = p.substr(0, pos);
    sn = p.substr(pos + 1);
  }
  int64_t sec = 0;
  auto res = std::from_chars(ss.data(), ss.data() + ss.size(), sec);
  if (res.ec != std::errc{}) {
    ec = bela::make_error_code(bela::ErrGeneral, L"unable parse number '", bela::ToWide(ss), L"'");
    return false;
  }
  if (sn.empty()) {
    t = bela::FromUnix(sec, 0);
    return true;
  }
  /* Calculate nanoseconds. */
  int64_t nsec{0};
  std::from_chars(ss.data(), ss.data() + ss.size(), sec);
  t = bela::FromUnix(sec, nsec);
  return true;
}

bool Reader::mergePAX(Header &h, pax_records_t &paxHdrs, bela::error_code &ec) {
  for (const auto &[k, v] : paxHdrs) {
    if (v.empty()) {
      continue;
    }
    if (k == paxPath) {
      h.Name = v;
      continue;
    }
    if (k == paxLinkpath) {
      h.LinkName = v;
      continue;
    }
    if (k == paxUname) {
      h.Uname = v;
      continue;
    }
    if (k == paxGname) {
      h.Gname = v;
      continue;
    }
    if (k == paxUid) {
      int64_t id{0};
      auto res = std::from_chars(v.data(), v.data() + v.size(), id);
      if (res.ec != std::errc{}) {
        ec = bela::make_error_code(bela::ErrGeneral, L"unable parse uid number '", bela::ToWide(v), L"'");
        return false;
      }
      h.UID = static_cast<int>(id);
      continue;
    }
    if (k == paxGid) {
      int64_t id{0};
      auto res = std::from_chars(v.data(), v.data() + v.size(), id);
      if (res.ec != std::errc{}) {
        ec = bela::make_error_code(bela::ErrGeneral, L"unable parse gid number '", bela::ToWide(v), L"'");
        return false;
      }
      h.GID = static_cast<int>(id);
      continue;
    }
    if (k == paxAtime) {
      if (parsePAXTime(v, h.AccessTime, ec)) {
        return false;
      }
      continue;
    }
    if (k == paxMtime) {
      if (parsePAXTime(v, h.ModTime, ec)) {
        return false;
      }
      continue;
    }
    if (k == paxCtime) {
      if (parsePAXTime(v, h.ChangeTime, ec)) {
        return false;
      }
      continue;
    }
    if (k == paxSize) {
      int64_t size{0};
      auto res = std::from_chars(v.data(), v.data() + v.size(), size);
      if (res.ec != std::errc{}) {
        ec = bela::make_error_code(bela::ErrGeneral, L"unable parse size number '", bela::ToWide(v), L"'");
        return false;
      }
      h.Size = size;
      continue;
    }
    if (k.starts_with(paxSchilyXattr)) {
      h.Xattrs.emplace(k.substr(paxSchilyXattr.size()), v);
    }
  }
  h.PAXRecords = std::move(paxHdrs);
  return true;
}

bool Reader::parsePAX(int64_t paxSize, pax_records_t &paxHdrs, bela::error_code &ec) {
  Buffer buf(paxSize);
  if (!ReadFull(buf.data(), paxSize, ec)) {
    return false;
  }

  return true;
}

std::optional<Header> Reader::Next(bela::error_code &ec) {
  pax_records_t paxHdrs;
  std::string gnuLongName;
  std::string gnuLongLink;
  // read next entry
  for (;;) {
    if (!discard(remainingSize, ec)) {
      return std::nullopt;
    }
    if (!discard(paddingSize, ec)) {
      return std::nullopt;
    }
    Header h;
    if (!readHeader(h, ec)) {
      return std::nullopt;
    }
    if (h.Typeflag == TypeXHeader || h.Typeflag == TypeXGlobalHeader) {
      h.Format &= FormatPAX;
      if (!parsePAX(h.Size, paxHdrs, ec)) {
        return std::nullopt;
      }
      if (h.Typeflag == TypeXGlobalHeader) {
        mergePAX(h, paxHdrs, ec);
        Header nh;
        nh.Name = h.Name;
        nh.Typeflag = h.Typeflag;
        nh.Xattrs = h.Xattrs;
        nh.PAXRecords = h.PAXRecords;
        nh.Format = h.Format;
        return std::make_optional(std::move(nh));
      }
      continue;
    }
    return std::make_optional(std::move(h));
  }

  return std::nullopt;
}

} // namespace baulk::archive::tar