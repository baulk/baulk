///
#ifndef BAULK_ARCHIVE_IO_HPP
#define BAULK_ARCHIVE_IO_HPP
#include <bela/base.hpp>
#include <bela/types.hpp>
#include <bela/time.hpp>
#include <functional>

namespace baulk::archive {
using bela::ssize_t;
using Writer = std::function<bool(const void *data, size_t len, bela::error_code &ec)>;
struct Reader {
  virtual ssize_t Read(void *buffer, size_t len, bela::error_code &ec) = 0;
  virtual bool Discard(int64_t len, bela::error_code &ec) = 0;
  // Avoid multiple memory copies
  virtual bool WriteTo(const Writer &w, int64_t filesize, int64_t &extracted, bela::error_code &ec) = 0;
  virtual bool Reset(bela::error_code &ec) = 0;
};

class FileReader : public Reader {
public:
  FileReader(HANDLE fd_, int64_t offset_ = 0, int64_t size_ = bela::SizeUnInitialized, bool nc = false)
      : fd(fd_), base{offset_}, size(size_), needClosed(nc) {}
  FileReader(const FileReader &) = delete;
  FileReader &operator=(const FileReader &) = delete;
  ~FileReader();
  ssize_t Read(void *buffer, size_t len, bela::error_code &ec);
  bool Discard(int64_t len, bela::error_code &ec);
  bool WriteTo(const Writer &w, int64_t filesize, int64_t &extracted, bela::error_code &ec);
  bool Reset(bela::error_code &ec) = 0;

private:
  HANDLE fd{INVALID_HANDLE_VALUE};
  int64_t base{0};
  int64_t size{bela::SizeUnInitialized};
  bool needClosed{false};
};

} // namespace baulk::archive

#endif