//
#include "tarinternal.hpp"
#include <bela/str_cat_narrow.hpp>
#include <bela/str_join_narrow.hpp>
#include <charconv>

namespace baulk::archive::tar {
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

// ReadFull read full data
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

bool Reader::parsePAX(int64_t paxSize, pax_records_t &paxHdrs, bela::error_code &ec) {
  Buffer buf(paxSize);
  if (!ReadFull(buf.data(), paxSize, ec)) {
    return false;
  }
  std::string_view sv{reinterpret_cast<const char *>(buf.data()), static_cast<size_t>(paxSize)};
  std::vector<std::string_view> sparseMap;
  while (!sv.empty()) {
    //
  }
  if (!sparseMap.empty()) {
    paxHdrs.emplace(paxGNUSparseMap, bela::narrow::StrJoin(sparseMap, ","));
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