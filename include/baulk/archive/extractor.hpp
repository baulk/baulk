///
#ifndef BAULK_ARCHIVE_EXTRACTOR_HPP
#define BAULK_ARCHIVE_EXTRACTOR_HPP
#include <bela/base.hpp>
#include <bela/io.hpp>
#include <baulk/archive.hpp>
#include <baulk/archive/zip.hpp>
#include <functional>

namespace baulk::archive {
// Options
struct ExtractorOptions {
  bool ignore_error{false};
  bool overwrite_mode{true};
};

namespace zip {
using Filter = std::function<bool(const baulk::archive::zip::File &file, std::wstring_view relative_name)>;
using OnProgress = std::function<bool(size_t bytes)>;
class Extractor {
public:
  Extractor(const ExtractorOptions &opts_) noexcept : opts(opts_) {}
  Extractor(const Extractor &) = delete;
  Extractor &operator=(const Extractor &) = delete;
  auto UncompressedSize() const { return reader.UncompressedSize(); }
  auto CompressedSize() const { return reader.CompressedSize(); }
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
  bool Extract(const Filter &filter, const OnProgress &progress, bela::error_code &ec) {
    std::error_code e;
    if (std::filesystem::create_directories(destination, e); e) {
      ec = bela::from_std_error_code(e, L"fs::create_directories() ");
      return false;
    }
    for (const auto &file : reader.Files()) {
      if (!extract_entry(file, filter, progress, ec)) {
        if (ec.code == bela::ErrCanceled || opts.ignore_error == false) {
          return false;
        }
      }
    }
    return true;
  }

private:
  Reader reader;
  std::filesystem::path destination;
  ExtractorOptions opts;
  bool extract_entry(const baulk::archive::zip::File &file, const Filter &filter, const OnProgress &progress,
                     bela::error_code &ec) {
    std::wstring encoded_path;
    auto out = baulk::archive::JoinSanitizeFsPath(destination, file.name, file.IsFileNameUTF8(), encoded_path);
    if (!out) {
      ec = bela::make_error_code(bela::ErrGeneral, L"harmful path: ", bela::encode_into<char, wchar_t>(file.name));
      return false;
    }
    if (filter && !filter(file, encoded_path)) {
      ec = bela::make_error_code(bela::ErrCanceled, L"canceled");
      return false;
    }
    std::error_code e;
    if (file.IsDir()) {
      if (std::filesystem::create_directories(*out, e); e) {
        ec = bela::from_std_error_code(e, L"fs::create_directories() ");
        return false;
      }
      baulk::archive::Chtimes(*out, file.time, ec);
      return true;
    }
    if (file.IsSymlink()) {
      std::wstring encoded_linkname;
      auto source_path = baulk::archive::JoinSanitizeFsPath(out->parent_path(), reader.ResolveLinkName(file, ec),
                                                            file.IsFileNameUTF8(), encoded_linkname);
      if (!source_path) {
        ec = bela::make_error_code(bela::ErrGeneral, L"harmful path: ", bela::encode_into<char, wchar_t>(file.name));
        return false;
      }
      return baulk::archive::NewSymlink(*out, *source_path, opts.overwrite_mode, ec);
    }
    auto fd = baulk::archive::File::NewFile(*out, file.time, opts.overwrite_mode, ec);
    if (!fd) {
      return false;
    }
    bela::error_code writeEc;
    return reader.Decompress(
        file,
        [&](const void *data, size_t len) {
          if (progress && !progress(len)) {
            // canceled
            return false;
          }
          return fd->WriteFull(data, len, writeEc);
        },
        ec);
  }
};
} // namespace zip
namespace tar {
class Extractor {};
} // namespace tar
} // namespace baulk::archive

#endif