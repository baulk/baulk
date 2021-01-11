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
  bela::os::FileMode mode;
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

class Reader {
public:
  Reader(bela::io::Reader *r_) : r(r_) {}
  Reader(const Reader &) = delete;
  Reader &operator=(const Reader &) = delete;
  std::optional<File> Next(bela::error_code &ec);
  bela::ssize_t Read(void *buffer, size_t size, bela::error_code &ec);
  bela::ssize_t ReadAtLeast(void *buffer, size_t size, bela::error_code &ec);

private:
  bela::io::Reader *r{nullptr};
};
} // namespace baulk::archive::tar

#endif