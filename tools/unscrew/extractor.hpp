///
#ifndef EXTRACTOR_HPP
#define EXTRACTOR_HPP
#include <bela/base.hpp>
#include <bela/io.hpp>
#include <bela/comutils.hpp>
#include <baulk/archive/zip.hpp>
#include <ShObjIdl.h>
#include <ShlObj_core.h>
#include <filesystem>
#include <functional>

namespace baulk {
using Filter = std::function<bool(const baulk::archive::zip::File &file, std::wstring_view relative_name)>;
using OnEntries = std::function<bool(std::wstring_view filename)>;
using ZipReader = baulk::archive::zip::Reader;
class ZipExtractor {
public:
  ZipExtractor() = default;
  ZipExtractor(const ZipExtractor &) = delete;
  ZipExtractor &operator=(const ZipExtractor &) = delete;
  bool OpenReader(const std::filesystem::path &file, const std::filesystem::path &dest, bela::error_code &ec) {
    std::error_code e;
    if (destination = std::filesystem::absolute(dest, e); e) {
      ec = bela::from_std_error_code(e, L"fs::absolute() ");
      return false;
    }
    auto zipfile = std::filesystem::canonical(file, e);
    if (e) {
      ec = bela::from_std_error_code(e, L"fs::canonical() ");
      return false;
    }
    return reader.OpenReader(zipfile.c_str(), ec);
  }
  bool OpenReader(bela::io::FD &fd, const std::filesystem::path &dest, int64_t size, int64_t offset,
                  bela::error_code &ec) {
    std::error_code e;
    if (destination = std::filesystem::absolute(dest, e); e) {
      ec = bela::from_std_error_code(e, L"fs::absolute() ");
      return false;
    }
    return reader.OpenReader(fd.NativeFD(), size, offset, ec);
  }
  bool Extract(const Filter &filter, bela::error_code &ec) {
    std::error_code e;
    if (std::filesystem::create_directories(destination, e); e) {
      ec = bela::from_std_error_code(e, L"fs::create_directories() ");
      return false;
    }
    for (const auto &file : reader.Files()) {
      if (!extract_entry(filter, file, ec)) {
        return false;
      }
    }
    return true;
  }

private:
  ZipReader reader;
  std::filesystem::path destination;
  bool extract_entry(const Filter &filter, const baulk::archive::zip::File &file, bela::error_code &ec) {
    //

    return false;
  }
};

class Extractor {
public:
  Extractor() = default;
  Extractor(const Extractor &) = delete;
  Extractor &operator=(const Extractor &) = delete;
  bool ParseArgv(bela::error_code &ec);
  bool Execute(bela::error_code &ec);

private:
  std::vector<std::filesystem::path> archives;
  // bela::comptr<IProgressDialog> bar;
};
} // namespace baulk

#endif