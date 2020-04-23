///
#ifndef BAULK_ZIP_HPP
#define BAULK_ZIP_HPP
#include <cstdint>
#include <bela/base.hpp>
// https://www.winzip.com/win/en/comp_info.html
extern "C" {
struct mz_zip_reader_s;
typedef mz_zip_reader_s mz_zip_reader;
}

namespace baulk::archive::zip {
class Decompressor {
public:
  Decompressor() = default;
  Decompressor(const Decompressor &) = delete;
  Decompressor &operator=(const Decompressor &) = delete;
  ~Decompressor();
  bool Stealing(HANDLE &FileHandle, bela::error_code &ec);
  bool ZipDecompress(std::wstring_view dest, bela::error_code &ec);

private:
  int Decompress(std::wstring_view path, bela::error_code &ec);
  mz_zip_reader *reader{nullptr};
};

} // namespace baulk::archive::zip

#endif