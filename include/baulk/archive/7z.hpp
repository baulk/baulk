//
#ifndef BAULK_ARCHIVE_7Z_HPP
#define BAULK_ARCHIVE_7Z_HPP
#include <bela/io.hpp>

namespace baulk::archive::n7z {
class Reader {
public:
  Reader() = default;
  Reader(const Reader &) = delete;
  Reader &operator=(const Reader &) = delete;
  bool OpenReader(std::wstring_view file, bela::error_code &ec);
  bool OpenReader(HANDLE nfd, int64_t size_, int64_t offset_, bela::error_code &ec);

private:
  bela::io::FD fd;
  int64_t size{bela::SizeUnInitialized};
  int64_t startPosition{0};
};
} // namespace baulk::archive::n7z

#endif