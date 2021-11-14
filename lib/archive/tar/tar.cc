//
#include "tarinternal.hpp"
#include <bela/str_cat_narrow.hpp>
#include <bela/str_join_narrow.hpp>
#include <bela/str_split_narrow.hpp>
#include <baulk/allocate.hpp>
#include <charconv>

namespace baulk::archive::tar {
// tar code
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

bool FileReader::Discard(int64_t len, bela::error_code &ec) {
  auto li = *reinterpret_cast<LARGE_INTEGER *>(&len);
  LARGE_INTEGER oli{0};
  if (SetFilePointerEx(fd, li, &oli, SEEK_CUR) != TRUE) {
    ec = bela::make_system_error_code(L"SetFilePointerEx: ");
    return false;
  }
  return true;
}

bool FileReader::WriteTo(const Writer &w, int64_t filesize, int64_t &extracted, bela::error_code &ec) {
  constexpr int64_t bufferSize = 8192;
  char buffer[bufferSize];
  while (filesize > 0) {
    auto minsize = (std::min)(bufferSize, filesize);
    DWORD drSize = {0};
    if (::ReadFile(fd, buffer, static_cast<DWORD>(minsize), &drSize, nullptr) != TRUE) {
      ec = bela::make_system_error_code(L"ReadFile: ");
      return false;
    }
    filesize -= drSize;
    extracted += drSize;
    if (!w(buffer, drSize, ec)) {
      return false;
    }
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

inline bool handleRegularFile(Header &h, int64_t &padding, bela::error_code &ec) {
  auto nb = h.Size;
  if (isHeaderOnlyType(h.Typeflag)) {
    nb = 0;
  }
  if (nb < 0) {
    ec = bela::make_error_code(L"invalid tar header");
    return false;
  }
  padding = blockPadding(nb);
  return true;
}

bela::ssize_t Reader::readInternal(void *buffer, size_t size, bela::error_code &ec) {
  if (r == nullptr) {
    ec = bela::make_error_code(L"underlying reader is null");
    return -1;
  }
  return r->Read(buffer, size, ec);
}

bool Reader::discard(int64_t bytes, bela::error_code &ec) {
  if (r == nullptr) {
    ec = bela::make_error_code(L"underlying reader is null");
    return false;
  }
  return r->Discard(bytes, ec);
}

bela::ssize_t Reader::Read(void *buffer, size_t size, bela::error_code &ec) {
  auto n = readInternal(buffer, size, ec);
  if (n > 0 && remainingSize > 0) {
    remainingSize -= n;
  }
  return n;
}

// ReadFull read full data
bool Reader::ReadFull(void *buffer, size_t size, bela::error_code &ec) {
  size_t rbytes = 0;
  auto p = reinterpret_cast<uint8_t *>(buffer);
  while (rbytes < size) {
    auto sz = readInternal(p + rbytes, size - rbytes, ec);
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

bool Reader::parsePAX(int64_t paxSize, pax_records_t &paxHdrs, bela::error_code &ec) {
  Buffer buf(paxSize);
  if (!ReadFull(buf.data(), paxSize, ec)) {
    return false;
  }
  std::string_view sv{reinterpret_cast<const char *>(buf.data()), static_cast<size_t>(paxSize)};
  std::vector<std::string_view> sparseMap;
  while (!sv.empty()) {
    std::string_view k;
    std::string_view v;
    if (!parsePAXRecord(&sv, &k, &v, ec)) {
      return false;
    }
    if (k == paxGNUSparseOffset || k == paxGNUSparseNumBytes) {
      if ((sparseMap.size() % 2 == 0 && k != paxGNUSparseOffset) ||
          (sparseMap.size() % 2 == 1 && k != paxGNUSparseNumBytes) || v.find('.') != std::string_view::npos) {
        ec = bela::make_error_code(L"invalid tar header");
        return false;
      }
      sparseMap.emplace_back(v);
      continue;
    }
    paxHdrs.emplace(k, v);
  }
  if (!sparseMap.empty()) {
    paxHdrs.emplace(paxGNUSparseMap, bela::narrow::StrJoin(sparseMap, ","));
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
  h.Mode = parseNumeric(hdr.mode);
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
  return true;
}

bool Reader::readOldGNUSparseMap(Header &h, sparseDatas &spd, const gnutar_header *th, bela::error_code &ec) {
  ec = bela::make_error_code(L"tar: GNU old sparse file extraction is not implemented");
  return false;
}

bool readGNUSparseMap0x1(pax_records_t &paxrs, sparseDatas &spd, bela::error_code &ec) {
  auto it = paxrs.find(paxGNUSparseNumBlocks);
  if (it == paxrs.end()) {
    ec = bela::make_error_code(L"tar: pax sparse missing num blocks");
    return false;
  }
  int64_t numEntries{0};
  if (!bela::SimpleAtoi(it->second, &numEntries) || numEntries < 0 ||
      static_cast<int>(numEntries * 2) < static_cast<int>(numEntries)) {
    ec = bela::make_error_code(L"tar: pax sparse invalid num blocks");
    return false;
  }
  auto iter = paxrs.find(paxGNUSparseMap);
  if (iter == paxrs.end()) {
    ec = bela::make_error_code(L"tar: pax sparse missing sparse map");
    return false;
  }
  std::vector<std::string_view> sparseMap = bela::narrow::StrSplit(iter->second, bela::narrow::ByChar(','));
  if (sparseMap.size() == 1 && sparseMap[0] == "") {
    sparseMap.clear();
  }
  if (sparseMap.size() != static_cast<size_t>(2 * numEntries)) {
    ec = bela::make_error_code(L"tar: pax sparse invalid sparse map size");
    return false;
  }
  spd.resize(numEntries);
  for (int64_t i = 0; i < numEntries; i++) {
    // 0,0,1 1,2,3 2,4,5 3,6,7 4,8,9
    if (!bela::SimpleAtoi(sparseMap[2 * i], &spd[i].Offset) ||
        !bela::SimpleAtoi(sparseMap[2 * i + 1], &spd[i].Length)) {
      ec = bela::make_error_code(L"tar: pax sparse invalid sparse offset or length");
      return false;
    }
  }
  return true;
}

bool Reader::readGNUSparseMap1x0(sparseDatas &spd, bela::error_code &ec) {
  int64_t cntNewline{0};
  char block[512];
  std::string buf;
  std::string::size_type offset{0};
  auto feedTokens = [&](int64_t n, bela::error_code &e) -> bool {
    while (cntNewline < n) {
      if (!ReadFull(block, sizeof(block), ec)) {
        return false;
      }
      buf.append(block, sizeof(block));
      for (auto c : block) {
        if (c == '\n') {
          cntNewline++;
        }
      }
    }
    return true;
  };
  auto nextToken = [&]() -> std::string_view {
    cntNewline--;
    auto pos = buf.find('\n', offset);
    if (pos != std::string::npos) {
      std::string_view sv{buf.data() + offset, pos};
      offset = pos + 1;
      return sv;
    }
    return std::string_view{buf.data() + offset, buf.size() - offset};
  };
  if (!feedTokens(1, ec)) {
    return false;
  }
  auto nextTokenStr = nextToken();
  int64_t numEntries{0};
  if (!bela::SimpleAtoi(nextTokenStr, &numEntries) || numEntries < 0 ||
      static_cast<int>(numEntries * 2) < static_cast<int>(numEntries)) {
    ec = bela::make_error_code(L"tar: pax sparse invalid num blocks");
    return false;
  }
  if (!feedTokens(2 * numEntries, ec)) {
    return false;
  }
  spd.resize(numEntries);
  for (int64_t i = 0; i < numEntries; i++) {
    if (!bela::SimpleAtoi(nextToken(), &spd[i].Offset) || !bela::SimpleAtoi(nextToken(), &spd[i].Length)) {
      ec = bela::make_error_code(L"tar: pax sparse invalid sparse offset or length");
      return false;
    }
  }
  return true;
}

bool Reader::readGNUSparsePAXHeaders(Header &h, sparseDatas &spd, bela::error_code &ec) {
  bool is1x0 = false;
  auto mit = h.PAXRecords.find(paxGNUSparseMajor);
  if (mit == h.PAXRecords.end()) {
    return true;
  }
  std::string_view major = mit->second;
  auto it = h.PAXRecords.find(paxGNUSparseMinor);
  if (it == h.PAXRecords.end()) {
    return false;
  }
  std::string_view minor = it->second;
  if (major == "0" && (minor == "0" || minor == "1")) {
    is1x0 = false;
  } else if (major == "1" && minor == "0") {
    is1x0 = true;
  } else if (major.empty() || minor.empty()) {
    return true;
  } else if (auto kit = h.PAXRecords.find(paxGNUSparseMap); kit != h.PAXRecords.end() && !kit->second.empty()) {
    is1x0 = false;
  } else {
    return true;
  }
  h.Format = FormatPAX;
  if (auto it = h.PAXRecords.find(paxGNUSparseName); it != h.PAXRecords.end() && !it->second.empty()) {
    h.Name = it->second;
  }
  if (auto it = h.PAXRecords.find(paxGNUSparseSize); it != h.PAXRecords.end() && !it->second.empty()) {
    int64_t n = 0;
    if (auto res = std::from_chars(it->second.data(), it->second.data() + it->second.size(), n);
        res.ec == std::errc{}) {
      h.SparseSize = n;
      return false;
    }
  }
  if (is1x0) {
    return readGNUSparseMap1x0(spd, ec);
  }
  return readGNUSparseMap0x1(h.PAXRecords, spd, ec);
}

// handleSparseFile tar support sparse file
// https://www.gnu.org/software/tar/manual/html_node/sparse.html
bool Reader::handleSparseFile(Header &h, const gnutar_header *gh, bela::error_code &ec) {
  std::vector<sparseEntry> spd;
  bool ret = false;
  if (h.Typeflag == TypeGNUSparse) {
    ret = readOldGNUSparseMap(h, spd, gh, ec);
  } else {
    ret = readGNUSparsePAXHeaders(h, spd, ec);
  }
  if (ret && !spd.empty()) {
    if (isHeaderOnlyType(h.Typeflag) || !validateSparseEntries(spd, h.Size)) {
      ec = bela::make_error_code(L"invalid tar header");
      return false;
    }
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
    if (!handleRegularFile(h, paddingSize, ec)) {
      return std::nullopt;
    }
    if (h.Typeflag == TypeXHeader || h.Typeflag == TypeXGlobalHeader) {
      h.Format &= FormatPAX;
      if (!parsePAX(h.Size, paxHdrs, ec)) {
        return std::nullopt;
      }
      if (h.Typeflag == TypeXGlobalHeader) {
        mergePAX(h, paxHdrs, ec);
        // C++20 designated initializers
        // https://mariusbancila.ro/blog/2020/02/27/c20-designated-initializers/
        // a designator must refer to a non-static direct data member
        // all the designator used in the initialization expression must follow the order of the declaration of the data
        // members in the class
        // not all data members must have a designator, but those that do must follow the rule above
        // it is not possible to mix designated and non-designated initialization
        // desginators of the same data member cannot appear multiple times
        // designators cannot be nested
        return std::make_optional(Header{.Name = h.Name,
                                         .Xattrs = h.Xattrs,
                                         .PAXRecords = h.PAXRecords,
                                         .Format = h.Format,
                                         .Typeflag = h.Typeflag});
      }
      continue;
    }
    if (h.Typeflag == TypeGNULongName || h.Typeflag == TypeGNULongLink) {
      h.Format = FormatGNU;
      Buffer realname(h.Size);
      if (!ReadFull(realname.data(), h.Size, ec)) {
        return std::nullopt;
      }
      if (h.Typeflag == TypeGNULongName) {
        gnuLongName = parseString(realname.data(), h.Size);
        continue;
      }
      gnuLongLink = parseString(realname.data(), h.Size);
      continue;
    }
    if (!mergePAX(h, paxHdrs, ec)) {
      return std::nullopt;
    }
    if (!gnuLongName.empty()) {
      h.Name = std::move(gnuLongName);
    }
    if (!gnuLongLink.empty()) {
      h.LinkName = std::move(gnuLongLink);
    }
    if (h.Typeflag == TypeRegA) {
      h.Typeflag = h.Name.ends_with('/') ? TypeDir : TypeReg;
    }
    if (!handleRegularFile(h, paddingSize, ec)) {
      return std::nullopt;
    }
    // TODO support tar sparse feature
    if ((h.Format & (FormatUSTAR | FormatPAX)) != 0) {
      h.Format = FormatUSTAR;
    }
    remainingSize = h.Size;
    return std::make_optional(std::move(h));
  }
  return std::nullopt;
}

bool Reader::WriteTo(const Writer &w, int64_t filesize, bela::error_code &ec) {
  ec.clear();
  int64_t extracted{0};
  auto ret = r->WriteTo(w, filesize, extracted, ec);
  if (remainingSize > 0) {
    remainingSize -= extracted;
  }
  return ret;
}

} // namespace baulk::archive::tar