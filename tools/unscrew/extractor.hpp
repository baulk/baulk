///
#ifndef EXTRACTOR_HPP
#define EXTRACTOR_HPP
#include <bela/base.hpp>
#include <bela/io.hpp>
#include <baulk/archive/zip.hpp>
#include <functional>

namespace baulk {
using OnEntries = std::function<bool(std::wstring_view filename)>;
class ZipExtractor {
public:
  ZipExtractor() = default;
  ZipExtractor(const ZipExtractor &) = delete;
  ZipExtractor &operator=(const ZipExtractor &) = delete;
  bool OpenReader(std::wstring_view file, std::wstring_view dest, bela::error_code &ec);
  bool OpenReader(bela::io::FD &fd, std::wstring_view dest, int64_t size, int64_t offset, bela::error_code &ec);

private:
};

class Extractor {
public:
  Extractor() = default;
  Extractor(const Extractor &) = delete;
  Extractor &operator=(const Extractor &) = delete;
  bool ParseArgv(bela::error_code &ec);
  bool Execute(bela::error_code &ec);

private:
};
} // namespace baulk

#endif