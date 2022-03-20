///
#ifndef BAULK_ARCHIVE_EXTRACTOR_HPP
#define BAULK_ARCHIVE_EXTRACTOR_HPP
#include <bela/base.hpp>
#include <bela/io.hpp>
#include <baulk/archive.hpp>
#include <baulk/archive/zip.hpp>
#include <baulk/archive/tar.hpp>
#include <functional>

namespace baulk::archive {
namespace fs = std::filesystem;
// Options
struct ExtractorOptions {
  bool ignore_error{false};
  bool overwrite_mode{true};
};

namespace zip {
using Filter = std::function<bool(const File &file, std::wstring_view relative_name)>;
using OnProgress = std::function<bool(size_t bytes)>;
class Extractor {
public:
  Extractor(const ExtractorOptions &opts_) noexcept : opts(opts_) {}
  Extractor(const Extractor &) = delete;
  Extractor &operator=(const Extractor &) = delete;
  auto UncompressedSize() const { return reader.UncompressedSize(); }
  auto CompressedSize() const { return reader.CompressedSize(); }
  bool OpenReader(const fs::path &file, const fs::path &dest, bela::error_code &ec) {
    std::error_code e;
    if (destination = fs::absolute(dest, e); e) {
      ec = bela::from_std_error_code(e, L"fs::absolute() ");
      return false;
    }
    auto zipfile = fs::canonical(file, e);
    if (e) {
      ec = bela::from_std_error_code(e, L"fs::canonical() ");
      return false;
    }
    return reader.OpenReader(zipfile.c_str(), ec);
  }
  bool OpenReader(bela::io::FD &fd, const fs::path &dest, int64_t size, int64_t offset, bela::error_code &ec) {
    std::error_code e;
    if (destination = fs::absolute(dest, e); e) {
      ec = bela::from_std_error_code(e, L"fs::absolute() ");
      return false;
    }
    return reader.OpenReader(fd.NativeFD(), size, offset, ec);
  }
  bool Extract(const Filter &filter, const OnProgress &progress, bela::error_code &ec) {
    std::error_code e;
    if (fs::create_directories(destination, e); e) {
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
  ExtractorOptions opts;
  Reader reader;
  fs::path destination;
  bool create_symlink(const fs::path &_New_symlink, std::string_view linkname, bool always_utf8, bela::error_code &ec) {
    if (baulk::archive::IsHarmfulPath(linkname)) {
      ec = bela::make_error_code(bela::ErrGeneral, L"harmful path: ", bela::encode_into<char, wchar_t>(linkname));
      return false;
    }
    std::filesystem::path linkPath(baulk::archive::EncodeToNativePath(linkname, always_utf8));
    if (linkPath.is_absolute()) {
      return baulk::archive::NewSymlink(_New_symlink, linkPath, opts.overwrite_mode, ec);
    }
    return baulk::archive::NewSymlink(_New_symlink, _New_symlink.parent_path() / linkPath, opts.overwrite_mode, ec);
  }

  bool extract_entry(const File &file, const Filter &filter, const OnProgress &progress, bela::error_code &ec) {
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
      return MakeDirectories(*out, file.time, ec);
    }
    if (file.IsSymlink()) {
      return create_symlink(*out, reader.ResolveLinkName(file, ec), file.IsFileNameUTF8(), ec);
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
using Filter = std::function<bool(const Header &hdr, std::wstring_view relative_name)>;
using OnProgress = std::function<bool(size_t bytes)>;

inline bool extractSymlink(std::wstring_view filename, std::string_view linkname, bela::error_code &ec) {
  return baulk::archive::NewSymlink(filename, bela::encode_into<char, wchar_t>(linkname), true, ec);
}

class Extractor {
public:
  Extractor(ExtractReader *r, const ExtractorOptions &opts_) noexcept : reader(r), opts(opts_) {}
  Extractor(const Extractor &) = delete;
  Extractor &operator=(const Extractor &) = delete;
  bool InitializeExtractor(const fs::path &dest, bela::error_code &ec) {
    std::error_code e;
    if (destination = fs::absolute(dest, e); e) {
      ec = bela::from_std_error_code(e, L"fs::absolute() ");
      return false;
    }
    if (reader == nullptr) {
      ec = bela::make_error_code(L"extract reader is nil");
      return false;
    }
    return true;
  }
  bool Extract(const Filter &filter, const OnProgress &progress, bela::error_code &ec) {
    std::error_code e;
    if (fs::create_directories(destination, e); e) {
      ec = bela::from_std_error_code(e, L"fs::create_directories() ");
      return false;
    }
    auto tr = std::make_shared<baulk::archive::tar::Reader>(reader);
    std::wstring encoded_path;
    for (;;) {
      auto fh = tr->Next(ec);
      if (!fh) {
        break;
      }
      if (extract_entry(*tr, *fh, filter, progress, ec)) {
        continue;
      }
      if (ec.code == bela::ErrCanceled) {
        return false;
      }
      if (ec.code == ErrNotTarFile || ec.code == ErrExtractGeneral || !opts.ignore_error) {
        break;
      }
    }
    if (tr->Index() == 0 && ec.code == ErrNotTarFile) {
      ec = bela::make_error_code(ErrAnotherWay, L"extract another way");
      return false;
    }
    if (ec && ec.code != bela::ErrEnded) {

      return false;
    }
    return true;
  }

private:
  ExtractReader *reader{nullptr};
  ExtractorOptions opts;
  fs::path destination;
  bool create_symlink(const fs::path &_New_symlink, std::string_view linkname, bela::error_code &ec) {
    if (baulk::archive::IsHarmfulPath(linkname)) {
      ec = bela::make_error_code(bela::ErrGeneral, L"harmful path: ", bela::encode_into<char, wchar_t>(linkname));
      return false;
    }
    std::filesystem::path linkPath(baulk::archive::EncodeToNativePath(linkname, true));
    if (linkPath.is_absolute()) {
      return baulk::archive::NewSymlink(_New_symlink, linkPath, opts.overwrite_mode, ec);
    }
    return baulk::archive::NewSymlink(_New_symlink, _New_symlink.parent_path() / linkPath, opts.overwrite_mode, ec);
  }
  bool extract_entry(Reader &tr, const Header &fh, const Filter &filter, const OnProgress &progress,
                     bela::error_code &ec) {
    std::wstring encoded_path;
    auto out = baulk::archive::JoinSanitizeFsPath(destination, fh.Name, true, encoded_path);
    if (!out) {
      ec = bela::make_error_code(bela::ErrGeneral, L"harmful path: ", bela::encode_into<char, wchar_t>(fh.Name));
      return false;
    }
    if (filter && !filter(fh, encoded_path)) {
      ec = bela::make_error_code(bela::ErrCanceled, L"canceled");
      return false;
    }
    if (fh.IsDir()) {
      return MakeDirectories(*out, fh.ModTime, ec);
    }
    if (fh.IsSymlink()) {
      return create_symlink(*out, fh.LinkName, ec);
    }
    // i
    if (!fh.IsRegular()) {
      return true;
    }
    auto fd = baulk::archive::File::NewFile(*out, fh.ModTime, true, ec);
    if (!fd) {
      return false;
    }
    if (!tr.WriteTo(
            [&](const void *data, size_t len, bela::error_code &ec) -> bool {
              if (progress && !progress(len)) {
                // canceled
                return false;
              }
              return fd->WriteFull(data, len, ec);
            },
            fh.Size, ec)) {
      fd->Discard();
      return false;
    }
    return true;
  }
};
} // namespace tar
} // namespace baulk::archive

#endif