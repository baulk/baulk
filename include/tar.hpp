/// Tar
#ifndef BAULK_TAR_HPP
#define BAULK_TAR_HPP
#include "archive.hpp"
#include <bela/io.hpp>
#include <memory>

namespace baulk::archive::tar {
constexpr long ErrNoneFilter = 754321;
using bela::ssize_t;
struct File {
  std::string name;
  int64_t size{0};
  bela::Time time;
  int64_t mode;
  uint32_t chcksum{0};
};

class FileReader : public bela::io::ReaderAt {
public:
  FileReader(HANDLE fd_, bool nc = false) : fd(fd_), needClosed(nc) {}
  FileReader(const FileReader &) = delete;
  FileReader &operator=(const FileReader &) = delete;
  ~FileReader();
  ssize_t Read(void *buffer, size_t len, bela::error_code &ec);
  ssize_t ReadAt(void *buffer, size_t len, int64_t pos, bela::error_code &ec);
  bool PositionAt(int64_t pos, bela::error_code &ec);

private:
  HANDLE fd{INVALID_HANDLE_VALUE};
  bool needClosed{false};
};
std::shared_ptr<FileReader> OpenFile(std::wstring_view file, bela::error_code &ec);
std::shared_ptr<bela::io::Reader> MakeReader(FileReader &fd, bela::error_code &ec);

// ustar
struct ustar_header {       /* byte offset */
  char name[100];     /*   0 */
  char mode[8];       /* 100 */
  char uid[8];        /* 108 */
  char gid[8];        /* 116 */
  char size[12];      /* 124 */
  char mtime[12];     /* 136 */
  char chksum[8];     /* 148 */
  char typeflag;      /* 156 */
  char linkname[100]; /* 157 */
  char magic[6];      /* 257 */
  char version[2];    /* 263 */
  char uname[32];     /* 265 */
  char gname[32];     /* 297 */
  char devmajor[8];   /* 329 */
  char devminor[8];   /* 337 */
  char path[155];     /* 345 */
                      /* 500 */
  char padding[12];
};

class Reader {
public:
  Reader(bela::io::Reader *r_) : r(r_) {}
  Reader(const Reader &) = delete;
  Reader &operator=(const Reader &) = delete;
  std::optional<File> Next(bela::error_code &ec);
  bela::ssize_t Read(void *buffer, size_t size, bela::error_code &ec);
  bela::ssize_t ReadAtLeast(void *buffer, size_t size, bela::error_code &ec);

private:
  bool discard(bela::error_code &ec);
  bela::io::Reader *r{nullptr};
  int64_t paddingSize{0};
  ustar_header uhdr{0};
};

} // namespace baulk::archive::tar

#endif