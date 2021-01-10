/// Tar
#ifndef BAULK_TAR_HPP
#define BAULK_TAR_HPP
#include "archive.hpp"
#include <bela/io.hpp>

namespace baulk::archive::tar {
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
  FileReader(HANDLE fd_) : fd(fd_) {}
  ssize_t Read(void *buffer, size_t len, bela::error_code &ec);
  ssize_t ReadAt(void *buffer, size_t len, int64_t pos, bela::error_code &ec);

private:
  HANDLE fd{INVALID_HANDLE_VALUE};
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
  bela::io::Reader *r{nullptr};
};
} // namespace baulk::archive::tar

#endif