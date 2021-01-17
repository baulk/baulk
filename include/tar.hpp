/// Tar
#ifndef BAULK_TAR_HPP
#define BAULK_TAR_HPP
#include "archive.hpp"
#include <bela/io.hpp>
#include <bela/phmap.hpp>
#include <memory>

namespace baulk::archive::tar {
constexpr long ErrNoneFilter = 754321;
using bela::ssize_t;

enum tar_format_t : int {
  FormatUnknown = 0, // unknown
  FormatV7 = 1,
  FormatUSTAR = 2,
  FormatPAX = 4,
  FormatGNU = 8,
  FormatSTAR = 16,
  FormatMax = 32,
};

[[nodiscard]] constexpr tar_format_t operator&(tar_format_t L, tar_format_t R) noexcept {
  using I = std::underlying_type_t<tar_format_t>;
  return static_cast<tar_format_t>(static_cast<I>(L) & static_cast<I>(R));
}
[[nodiscard]] constexpr tar_format_t operator|(tar_format_t L, tar_format_t R) noexcept {
  using I = std::underlying_type_t<tar_format_t>;
  return static_cast<tar_format_t>(static_cast<I>(L) | static_cast<I>(R));
}

// Type '0' indicates a regular file.
constexpr char TypeReg = '0';
constexpr char TypeRegA = '\x00'; // Deprecated: Use TypeReg instead.

// Type '1' to '6' are header-only flags and may not have a data body.
constexpr char TypeLink = '1';    // Hard link
constexpr char TypeSymlink = '2'; // Symbolic link
constexpr char TypeChar = '3';    // Character device node
constexpr char TypeBlock = '4';   // Block device node
constexpr char TypeDir = '5';     // Directory
constexpr char TypeFifo = '6';    // FIFO node

// Type '7' is reserved.
constexpr char TypeCont = '7';

// Type 'x' is used by the PAX format to store key-value records that
// are only relevant to the next file.
// This package transparently handles these types.
constexpr char TypeXHeader = 'x';

// Type 'g' is used by the PAX format to store key-value records that
// are relevant to all subsequent files.
// This package only supports parsing and composing such headers,
// but does not currently support persisting the global state across files.
constexpr char TypeXGlobalHeader = 'g';

// Type 'S' indicates a sparse file in the GNU format.
constexpr char TypeGNUSparse = 'S';

// Types 'L' and 'K' are used by the GNU format for a meta file
// used to store the path or link name for the next file.
// This package transparently handles these types.
constexpr char TypeGNULongName = 'L';
constexpr char TypeGNULongLink = 'K';

// https://www.mkssoftware.com/docs/man4/tar.4.asp
/*  ustar
 *    File Header (512 bytes)
 *  Offst Size Field
 *      Pre-POSIX Header
 *  0     100  File name
 *  100   8    File mode
 *  108   8    Owner's numeric user ID
 *  116   8    Group's numeric user ID
 *  124   12   File size in bytes (octal basis)
 *  136   12   Last modification time in numeric Unix time format (octal)
 *  148   8    Checksum for header record
 *  156   1    Type flag
 *  157   100  Name of linked file
 *      UStar Format
 *  257   6    UStar indicator "ustar"
 *  263   2    UStar version "00"
 *  265   32   Owner user name
 *  297   32   Owner group name
 *  329   8    Device major number
 *  337   8    Device minor number
 *  345   155  Filename prefix
 */
struct ustar_header {
  char name[100];
  char mode[8];
  char uid[8];
  char gid[8];
  char size[12];
  char mtime[12];
  char chksum[8];
  char typeflag;
  char linkname[100];
  char magic[6];
  char version[2];
  char uname[32];
  char gname[32];
  char devmajor[8];
  char devminor[8];
  char prefix[155];
  char padding[12]; // 512 byte aligned padding
};

/*
 * Structure of GNU tar header
 */
struct gnu_sparse {
  char offset[12];
  char numbytes[12];
};

struct gnutar_header {
  char name[100];
  char mode[8];
  char uid[8];
  char gid[8];
  char size[12];
  char mtime[12];
  char checksum[8];
  char typeflag[1];
  char linkname[100];
  char magic[8]; /* "ustar  \0" (note blank/blank/null at end) */
  char uname[32];
  char gname[32];
  char rdevmajor[8];
  char rdevminor[8];
  char atime[12];
  char ctime[12];
  char offset[12];
  char longnames[4];
  char unused[1];
  gnu_sparse sparse[4];
  char isextended[1];
  char realsize[12];
  /*
   * Old GNU format doesn't use POSIX 'prefix' field; they use
   * the 'L' (longname) entry instead.
   */
  char padding[17];
};

// GNU sparse entry
struct sparseEntry {
  int64_t Offset{0};
  int64_t Length{0};
  int64_t endOffset() const { return Offset + Length; }
};

using sparseDatas = std::vector<sparseEntry>;

using pax_records_t = bela::flat_hash_map<std::string, std::string>;

struct Header {
  std::string Name;
  std::string LinkName;
  std::string Uname;
  std::string Gname;
  int64_t Size{0};
  int64_t SparseSize{0};
  int64_t Mode{0};
  bela::Time ModTime;
  bela::Time AccessTime;
  bela::Time ChangeTime;
  int64_t Devmajor{0};
  int64_t Devminor{0};
  pax_records_t Xattrs;
  pax_records_t PAXRecords;
  int UID{0};
  int GID{0};
  int Format{0};
  char Typeflag{0};
};

class FileReader : public bela::io::ReaderAt {
public:
  FileReader(HANDLE fd_, int64_t size_ = bela::SizeUnInitialized, bool nc = false)
      : fd(fd_), size(size_), needClosed(nc) {}
  FileReader(const FileReader &) = delete;
  FileReader &operator=(const FileReader &) = delete;
  ~FileReader();
  ssize_t Read(void *buffer, size_t len, bela::error_code &ec);
  ssize_t ReadAt(void *buffer, size_t len, int64_t pos, bela::error_code &ec);
  bool PositionAt(int64_t pos, bela::error_code &ec);
  int64_t Size() const { return size; }

private:
  HANDLE fd{INVALID_HANDLE_VALUE};
  int64_t size{bela::SizeUnInitialized};
  bool needClosed{false};
};
std::shared_ptr<FileReader> OpenFile(std::wstring_view file, bela::error_code &ec);
std::shared_ptr<bela::io::Reader> MakeReader(FileReader &fd, bela::error_code &ec);

class Reader {
public:
  Reader(bela::io::Reader *r_) : r(r_) {}
  Reader(const Reader &) = delete;
  Reader &operator=(const Reader &) = delete;
  std::optional<Header> Next(bela::error_code &ec);
  bela::ssize_t Read(void *buffer, size_t size, bela::error_code &ec);
  bool ReadFull(void *buffer, size_t size, bela::error_code &ec);

private:
  bela::ssize_t readInternal(void *buffer, size_t size, bela::error_code &ec);
  bool discard(int64_t bytes, bela::error_code &ec);
  bool readHeader(Header &h, bela::error_code &ec);
  bool parsePAX(int64_t paxSize, pax_records_t &paxHdrs, bela::error_code &ec);
  bool handleSparseFile(Header &h, const gnutar_header *th, bela::error_code &ec);
  bool readOldGNUSparseMap(Header &h, const gnutar_header *th, bela::error_code &ec);
  bool readGNUSparsePAXHeaders(Header &h, sparseDatas &spd, bela::error_code &ec);
  bela::io::Reader *r{nullptr};
  int64_t remainingSize{0};
  int64_t paddingSize{0};
};
std::wstring_view PathRemoveExtension(std::wstring_view p);
} // namespace baulk::archive::tar

#endif