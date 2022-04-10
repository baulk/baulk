///
#include <bela/path.hpp>
#include <bela/endian.hpp>
#include <bela/bufio.hpp>
#include <bitset>
#include <bela/terminal.hpp>
#include "zipinternal.hpp"

namespace hazel::zip {

int findSignatureInBlock(const bela::Buffer &b) {
  for (auto i = static_cast<int>(b.size()) - directoryEndLen; i >= 0; i--) {
    if (b[i] == 'P' && b[i + 1] == 'K' && b[i + 2] == 0x05 && b[i + 3] == 0x06) {
      auto n = static_cast<int>(b[i + directoryEndLen - 2]) | (static_cast<int>(b[i + directoryEndLen - 1]) << 8);
      if (n + directoryEndLen + i <= static_cast<int>(b.size())) {
        return i;
      }
    }
  }
  return -1;
}
bool Reader::readDirectory64End(int64_t offset, directoryEnd &d, bela::error_code &ec) {
  uint8_t buffer[256];
  if (!fd.ReadAt({buffer, directory64EndLen}, offset, ec)) {
    return false;
  }
  bela::endian::LittenEndian b({buffer, directory64EndLen});
  if (auto sig = b.Read<uint32_t>(); sig != directory64EndSignature) {
    ec = bela::make_error_code(L"zip: not a valid zip file");
    return false;
  }
  b.Discard(12);
  d.diskNbr = b.Read<uint32_t>();    // number of this disk
  d.dirDiskNbr = b.Read<uint32_t>(); // number of the disk with the start of the central directory
                                     // total number of entries in the central directory on this disk
  d.dirRecordsThisDisk = b.Read<uint64_t>();
  d.directoryRecords = b.Read<uint64_t>(); // total number of entries in the central directory
  d.directorySize = b.Read<uint64_t>();    // size of the central directory
  // offset of start of central directory with respect to the starting disk number
  d.directoryOffset = b.Read<uint64_t>();
  return true;
}

int64_t Reader::findDirectory64End(int64_t directoryEndOffset, bela::error_code &ec) {
  auto locOffset = directoryEndOffset - directory64LocLen;
  if (locOffset < 0) {
    return -1;
  }
  uint8_t buffer[256];
  if (!fd.ReadAt({buffer, directory64LocLen}, locOffset, ec)) {
    return -1;
  }
  bela::endian::LittenEndian b({buffer, directory64LocLen});
  if (auto sig = b.Read<uint32_t>(); sig != directory64LocSignature) {
    return -1;
  }
  if (b.Read<uint32_t>() != 0) {
    return -1;
  }
  auto p = b.Read<uint64_t>();
  if (b.Read<uint32_t>() != 1) {
    return -1;
  }
  return static_cast<int64_t>(p);
}

inline std::string cleanupName(const void *data, size_t N) {
  auto p = reinterpret_cast<const char *>(data);
  auto pos = memchr(p, 0, N);
  if (pos == nullptr) {
    return {p, N};
  }
  N = reinterpret_cast<const char *>(pos) - p;
  return {p, N};
}

// github.com\klauspost\compress@v1.11.3\zip\reader.go
bool Reader::readDirectoryEnd(directoryEnd &d, bela::error_code &ec) {
  bela::Buffer buffer(16 * 1024);
  int64_t directoryEndOffset = 0;
  constexpr size_t offrange[] = {1024, 65 * 1024};
  bela::endian::LittenEndian b;
  for (size_t i = 0; i < std::size(offrange); i++) {
    auto blen = offrange[i];
    if (static_cast<int64_t>(blen) > size) {
      blen = static_cast<size_t>(size);
    }
    buffer.grow(blen);
    if (!fd.ReadAt(buffer, blen, size - static_cast<int64_t>(blen), ec)) {
      return false;
    }
    if (auto p = findSignatureInBlock(buffer); p >= 0) {
      b.Reset({buffer.data() + p, static_cast<size_t>(blen - p)});
      directoryEndOffset = size - blen + p;
      break;
    }
    if (i == 1 || static_cast<int64_t>(blen) == size) {
      ec = bela::make_error_code(L"zip: not a valid zip file");
      return false;
    }
  }
  // 5*2 +4*2
  if (b.Discard(4) < 18) {
    ec = bela::make_error_code(L"zip: not a valid zip file");
    return false;
  }
  d.diskNbr = b.Read<uint16_t>();
  d.dirDiskNbr = b.Read<uint16_t>();
  d.dirRecordsThisDisk = b.Read<uint16_t>();
  d.directoryRecords = b.Read<uint16_t>();
  d.directorySize = b.Read<uint32_t>();
  d.directoryOffset = b.Read<uint32_t>();
  d.commentLen = b.Read<uint16_t>();
  if (static_cast<size_t>(d.commentLen) > b.Size()) {
    ec = bela::make_error_code(L"zip: invalid comment length");
    return false;
  }
  d.comment.assign(b.Data(), d.commentLen);
  if (d.directoryRecords == 0xFFFF || d.directorySize == 0xFFFF || d.directoryOffset == 0xFFFFFFFF) {
    ec.clear();
    auto p = findDirectory64End(directoryEndOffset, ec);
    if (!ec && p > 0) {
      readDirectory64End(p, d, ec);
    }
    if (ec) {
      return false;
    }
  }
  if (auto o = static_cast<int64_t>(d.directoryOffset); o < 0 || 0 >= size) {
    ec = bela::make_error_code(L"zip: not a valid zip file");
    return false;
  }
  return true;
}
using bufioReader = bela::bufio::Reader<4096>;

constexpr uint32_t SizeMin = 0xFFFFFFFFU;
constexpr uint64_t OffsetMin = 0xFFFFFFFFULL;

// Thanks github.com\klauspost\compress@v1.11.3\zip\reader.go

bool readDirectoryHeader(bufioReader &br, bela::Buffer &buffer, File &file, bela::error_code &ec) {
  uint8_t buf[directoryHeaderLen];
  if (br.ReadFull(buf, sizeof(buf), ec) != sizeof(buf)) {
    return false;
  }
  bela::endian::LittenEndian b(buf);
  if (auto n = static_cast<int>(b.Read<uint32_t>()); n != directoryHeaderSignature) {
    ec = bela::make_error_code(L"zip: not a valid zip file");
    return false;
  }
  file.version_madeby = b.Read<uint16_t>();
  file.version_needed = b.Read<uint16_t>();
  file.flags = b.Read<uint16_t>();
  file.method = b.Read<uint16_t>();
  auto dosTime = b.Read<uint16_t>();
  auto dosDate = b.Read<uint16_t>();
  file.crc32_value = b.Read<uint32_t>();
  file.compressed_size = b.Read<uint32_t>();
  file.uncompressed_size = b.Read<uint32_t>();
  auto filenameLen = b.Read<uint16_t>();
  auto extraLen = b.Read<uint16_t>();
  auto commentLen = b.Read<uint16_t>();
  b.Discard(4);
  auto externalAttrs = b.Read<uint32_t>();
  file.position = b.Read<uint32_t>();
  auto totallen = filenameLen + extraLen + commentLen;
  buffer.grow(totallen);
  if (br.ReadFull(buffer.data(), totallen, ec) != totallen) {
    return false;
  }
  buffer.size() = static_cast<size_t>(totallen);
  file.name = buffer.as_bytes_view().make_cstring_view(0, filenameLen);
  if (commentLen != 0) {
    file.comment = buffer.as_bytes_view().make_cstring_view(filenameLen + extraLen, commentLen);
  }
  auto needUSize = file.uncompressed_size == SizeMin;
  auto needSize = file.compressed_size == SizeMin;
  auto needOffset = file.position == OffsetMin;
  file.mode = resolveFileMode(file, externalAttrs);
  bela::Time modified;

  bela::endian::LittenEndian extra({buffer.data() + filenameLen, static_cast<size_t>(extraLen)});
  for (; extra.Size() >= 4;) {
    auto fieldTag = extra.Read<uint16_t>();
    auto fieldSize = static_cast<int>(extra.Read<uint16_t>());
    if (extra.Size() < static_cast<size_t>(fieldSize)) {
      break;
    }
    auto fb = extra.Sub(fieldSize);
    if (fieldTag == zip64ExtraID) {
      if (needUSize) {
        needUSize = false;
        if (fb.Size() < 8) {
          ec = bela::make_error_code(L"zip: not a valid zip file");
          return false;
        }
        file.uncompressed_size = fb.Read<uint64_t>();
      }
      if (needSize) {
        needSize = false;
        if (fb.Size() < 8) {
          ec = bela::make_error_code(L"zip: not a valid zip file");
          return false;
        }
        file.compressed_size = fb.Read<uint64_t>();
      }
      if (needOffset) {
        needOffset = false;
        if (fb.Size() < 8) {
          ec = bela::make_error_code(L"zip: not a valid zip file");
          return false;
        }
        file.position = fb.Read<uint64_t>();
      }
      continue;
    }
    if (fieldTag == ntfsExtraID) {
      if (fb.Size() < 4) {
        continue;
      }
      fb.Discard(4);
      for (; fb.Size() >= 4;) {
        auto attrTag = fb.Read<uint16_t>();
        auto attrSize = fb.Read<uint16_t>();
        if (fb.Size() < attrSize) {
          break;
        }
        auto ab = fb.Sub(attrSize);
        if (attrTag != 1 || attrSize != 24) {
          break;
        }
        modified = bela::FromWindowsPreciseTime(ab.Read<uint64_t>());
      }
      continue;
    }
    /*
       4.5.7 -UNIX Extra Field (0x000d):

        The following is the layout of the UNIX "extra" block.
        Note: all fields are stored in Intel low-byte/high-byte
        order.

        Value       Size          Description
        -----       ----          -----------
(UNIX)  0x000d      2 bytes       Tag for this "extra" block type
        TSize       2 bytes       Size for the following data block
        Atime       4 bytes       File last access time
        Mtime       4 bytes       File last modification time
        Uid         2 bytes       File user ID
        Gid         2 bytes       File group ID
        (var)       variable      Variable length data field

        The variable length data field will contain file type
        specific data.  Currently the only values allowed are
        the original "linked to" file names for hard or symbolic
        links, and the major and minor device node numbers for
        character and block device nodes.  Since device nodes
        cannot be either symbolic or hard links, only one set of
        variable length data is stored.  Link files will have the
        name of the original file stored.  This name is NOT NULL
        terminated.  Its size can be determined by checking TSize -
        12.  Device entries will have eight bytes stored as two 4
        byte entries (in little endian format).  The first entry
        will be the major device number, and the second the minor
        device number.
    */
    if (fieldTag == unixExtraID || fieldTag == infoZipUnixExtraID) {
      if (fb.Size() < 8) {
        continue;
      }
      fb.Discard(4);
      file.time = bela::FromUnixSeconds(static_cast<int64_t>(fb.Read<uint32_t>()));
      fb.Discard(4); // discard uid and gid
      if (fb.Size() > 0 && fieldTag == unixExtraID) {
        file.linkname = bela::cstring_view({fb.Data<char>() + 4, fb.Size()});
      }
      continue;
    }
    if (fieldTag == extTimeExtraID) {
      if (fb.Size() < 5) {
        continue;
      }
      auto flags = fb.Pick();
      if ((flags & 0x1) != 0) {
        modified = bela::FromUnixSeconds(static_cast<int64_t>(fb.Read<uint32_t>()));
        continue;
      }
      if ((flags & 0x2) != 0) {
        // atime: access time
        continue;
      }
      if ((flags & 0x4) != 0) {
        // ctime: create time
        continue;
      }
      continue;
    }
    if (fieldTag == infoZipUnicodePathID) {
      // 4.6.9 -Info-ZIP Unicode Path Extra Field (0x7075):

      /*
       (UPath) 0x7075        Short       tag for this extra block type ("up")
         TSize         Short       total data size for this block
         Version       1 byte      version of this extra field, currently 1
         NameCRC32     4 bytes     File Name Field CRC32 Checksum
         UnicodeName   Variable    UTF-8 version of the entry File Name
      */
      if (fb.Size() < 5 || (file.flags & 0x800) != 0) {
        continue;
      }
      (void)fb.Pick();
      auto crc32val = fb.Read<uint32_t>();
      (void)crc32val; // TODO
      file.flags |= 0x800;
      file.name = bela::cstring_view({fb.Data<char>(), fb.Size()});
      continue;
    }
    if (fieldTag == infoZipUnicodeCommentExtraID) {
      //  4.6.8 -Info-ZIP Unicode Comment Extra Field (0x6375):
      // (UCom) 0x6375        Short       tag for this extra block type ("uc")
      //  TSize         Short       total data size for this block
      //  Version       1 byte      version of this extra field, currently 1
      //  ComCRC32      4 bytes     Comment Field CRC32 Checksum
      //  UnicodeCom    Variable    UTF-8 version of the entry comment
      if (fb.Size() < 5) {
        continue;
      }
      (void)fb.Pick();
      auto crc32val = fb.Read<uint32_t>();
      (void)crc32val; // TODO
      file.comment = bela::cstring_view({fb.Data<char>(), fb.Size()});
      continue;
    }
    // https://www.winzip.com/win/en/aes_info.html
    if (fieldTag == winzipAesExtraID) {
      if (fb.Size() < 7) {
        continue;
      }
      file.aes_version = fb.Read<uint16_t>();
      fb.Discard(2); // VendorID 'AE'
      file.aes_strength = fb.Pick();
      file.method = fb.Read<uint16_t>();
      continue;
    }
    ///
  }
  file.time = bela::FromDosDateTime(dosDate, dosTime);
  if (bela::ToUnixSeconds(modified) != 0) {
    file.time = modified;
  }
  if (needSize || needOffset) {
    ec = bela::make_error_code(L"zip: not a valid zip file");
    return false;
  }
  return true;
}

bool Reader::Initialize(bela::error_code &ec) {
  if (size == bela::SizeUnInitialized) {
    if ((size = fd.Size(ec)) == bela::SizeUnInitialized) {
      return false;
    }
  }
  directoryEnd d;
  if (!readDirectoryEnd(d, ec)) {
    return false;
  }
  if (d.directoryRecords > static_cast<uint64_t>(size) / fileHeaderLen) {
    ec = bela::make_error_code(ErrGeneral, L"zip: TOC declares impossible ", d.directoryRecords, L" files in ", size,
                               L" byte zip");
    return false;
  }
  comment.assign(std::move(d.comment));
  files.reserve(static_cast<size_t>(d.directoryRecords));
  if (!fd.Seek(d.directoryOffset + baseOffset, ec)) {
    return false;
  }
  bela::Buffer buffer(16 * 1024);
  bufioReader br(fd.NativeFD());
  for (uint64_t i = 0; i < d.directoryRecords; i++) {
    File file;
    if (!readDirectoryHeader(br, buffer, file, ec)) {
      return false;
    }
    uncompressed_size += file.uncompressed_size;
    compressed_size += file.compressed_size;
    files.emplace_back(std::move(file));
  }
  return true;
}

bool Reader::OpenReader(std::wstring_view file, bela::error_code &ec) {
  auto fd_ = bela::io::NewFile(file, ec);
  if (!fd_) {
    return false;
  }
  fd = std::move(*fd_);
  return Initialize(ec);
}

bool Reader::OpenReader(HANDLE nfd, int64_t size_, bela::error_code &ec) {
  fd.Assgin(nfd, false);
  size = size_;
  return Initialize(ec);
}

bool Reader::OpenReader(HANDLE nfd, int64_t size_, int64_t offset_, bela::error_code &ec) {
  fd.Assgin(nfd, false);
  size = size_;
  baseOffset = offset_;
  return Initialize(ec);
}

std::wstring Method(uint16_t m) {
  struct method_kv_t {
    hazel::zip::zip_method_t m;
    const wchar_t *name;
  };
  constexpr const method_kv_t methods[] = {
      {zip_method_t::ZIP_STORE, L"store"},
      {zip_method_t::ZIP_SHRINK, L"shrunk"},
      {zip_method_t::ZIP_REDUCE_1, L"ZIP_REDUCE_1"},
      {zip_method_t::ZIP_REDUCE_2, L"ZIP_REDUCE_2"},
      {zip_method_t::ZIP_REDUCE_3, L"ZIP_REDUCE_3"},
      {zip_method_t::ZIP_REDUCE_4, L"ZIP_REDUCE_4"},
      {zip_method_t::ZIP_IMPLODE, L"IMPLODE"},
      {zip_method_t::ZIP_DEFLATE, L"deflate"},
      {zip_method_t::ZIP_DEFLATE64, L"deflate64"},
      {zip_method_t::ZIP_PKWARE_IMPLODE, L"ZIP_PKWARE_IMPLODE"},
      {zip_method_t::ZIP_BZIP2, L"bzip2"},
      {zip_method_t::ZIP_LZMA, L"lzma"},
      {zip_method_t::ZIP_TERSE, L"IBM TERSE"},
      {zip_method_t::ZIP_LZ77, L"LZ77"},
      {zip_method_t::ZIP_LZMA2, L"lzma2"},
      {zip_method_t::ZIP_ZSTD, L"zstd"},
      {zip_method_t::ZIP_XZ, L"xz"},
      {zip_method_t::ZIP_JPEG, L"Jpeg"},
      {zip_method_t::ZIP_WAVPACK, L"WavPack"},
      {zip_method_t::ZIP_PPMD, L"PPMd"},
      {zip_method_t::ZIP_AES, L"AES"},
      {zip_method_t::ZIP_BROTLI, L"brotli"},
  };
  for (const auto &i : methods) {
    if (static_cast<uint16_t>(i.m) == m) {
      return i.name;
    }
  }
  return std::wstring(bela::AlphaNum(m).Piece());
}

bool Reader::ContainsSlow(std::span<std::string_view> paths, std::size_t limit) const {
  size_t found = 0;
  bela::flat_hash_map<std::string_view, bool> pms;
  for (const auto p : paths) {
    pms.emplace(p, false);
  }
  auto maxsize = (std::min)(limit, files.size());
  for (size_t i = 0; i < maxsize; i++) {
    if (auto it = pms.find(files[i].name); it != pms.end()) {
      if (!it->second) {
        it->second = true;
        found++;
      }
    }
    if (found == paths.size()) {
      return true;
    }
  }
  return false;
}

bool Reader::Contains(std::span<std::string_view> paths, std::size_t limit) const {
  if (paths.empty() || paths.size() > files.size()) {
    return false;
  }
  if (paths.size() > 128) {
    return ContainsSlow(paths, limit);
  }
  std::bitset<128> mask;
  for (const auto &file : files) {
    for (size_t i = 0; i < paths.size(); i++) {
      if (file.name == paths[i]) {
        mask.set(i);
      }
    }
    if (mask.count() == paths.size()) {
      return true;
    }
  }
  return false;
}

bool Reader::Contains(std::string_view p, std::size_t limit) const {
  auto maxsize = (std::min)(limit, files.size());
  for (size_t i = 0; i < maxsize; i++) {
    if (files[i].name == p) {
      return true;
    }
  }
  return false;
}

zip_conatiner_t Reader::LooksLikeMsZipContainer() const {
  // [Content_Types].xml
  std::string_view paths[] = {"[Content_Types].xml", "_rels/.rels"};
  if (!Contains(paths, 200)) {
    return OfficeNone;
  }
  for (const auto &file : files) {
    if (file.StartsWith("word/")) {
      return OfficeDocx;
    }
    if (file.StartsWith("ppt/")) {
      return OfficePptx;
    }
    if (file.StartsWith("xl/")) {
      return OfficeXlsx;
    }
    if (!file.Contains('/') && file.EndsWith(".nuspec")) {
      return NuGetPackage;
    }
  }
  return OfficeNone;
}

bool Reader::LooksLikeOFD() const {
  std::string_view paths[] = {"OFD.xml", "Doc_1/DocumentRes.xml", "Doc_1/PublicRes.xml", "Doc_1/Annotations.xml",
                              "Doc_1/Document.xml"};
  return Contains(paths, 10000);
}

bool Reader::LooksLikeAppx() const {
  std::string_view paths[] = {"[Content_Types].xml", "AppxManifest.xml"};
  return Contains(paths);
}
bool Reader::LooksLikeApk() const {
  std::string_view paths[] = {"AndroidManifest.xml", "META-INF/MANIFEST.MF"};
  return Contains(paths);
}

bool Reader::LooksLikeJar() const {
  if (!Contains("META-INF/MANIFEST.MF")) {
    return false;
  }
  for (const auto &file : files) {
    if (bela::EndsWith(file.name, ".class")) {
      return true;
    }
  }
  return false;
}

bool Reader::LooksLikeODF(std::string *mime) const {
  std::string_view paths[] = {
      "META-INF/manifest.xml", "settings.xml", "content.xml", "styles.xml", "meta.xml", "mimetype",
  };
  if (!Contains(paths)) {
    return false;
  }
  if (mime == nullptr) {
    return true;
  }
  for (const auto &file : files) {
    if (file.name == "mimetype" && file.method == ZIP_STORE && file.compressed_size < 120) {
      bela::error_code ec;
      mime->reserve(static_cast<size_t>(file.compressed_size));
      return Decompress(
          file,
          [&](const void *data, size_t sz) -> bool {
            mime->append(static_cast<const char *>(data), sz);
            return true;
          },
          ec);
    }
  }
  return false;
}

} // namespace hazel::zip