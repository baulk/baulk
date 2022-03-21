#include <bela/path.hpp>
#include <baulk/fs.hpp>
#include <baulk/archive/extract.hpp>
#include "baulk.hpp"
#include "extract.hpp"

namespace baulk {
bool make_flattened(std::wstring_view path) {
  bela::error_code ec;
  // TODO some zip code
  return baulk::fs::MakeFlattened(path, path, ec);
}

bool extract_exe(std::wstring_view src, std::wstring_view dest, bela::error_code &ec) {
  if (!baulk::fs::MakeDirectories(dest, ec)) {
    return false;
  }
  auto fn = baulk::fs::FileName(src);
  auto newfile = bela::StringCat(dest, L"\\", fn);
  auto newfileold = bela::StringCat(newfile, L".old");
  if (bela::PathExists(newfile)) {
    if (MoveFileW(newfile.data(), newfileold.data()) != TRUE) {
      ec = bela::make_system_error_code();
      return false;
    }
  }
  if (MoveFileW(src.data(), newfile.data()) != TRUE) {
    ec = bela::make_system_error_code();
    return false;
  }
  return true;
}
bool make_flattened_exe(std::wstring_view path) {
  std::error_code ec;
  for (const auto &p : std::filesystem::directory_iterator(path)) {
    if (p.path().extension() == L".old") {
      std::filesystem::remove_all(p.path(), ec);
    }
  }
  return true;
}

bool extract_zip(std::wstring_view src, std::wstring_view dest, bela::error_code &ec) {
  baulk::archive::ZipExtractor extractor(baulk::IsQuietMode);
  if (!extractor.OpenReader(src, dest, ec)) {
    return false;
  }
  return extractor.Extract(ec);
}

inline bool extract_zip_simple(bela::io::FD &fd, std::wstring_view dest, int64_t offset, bela::error_code &ec) {
  baulk::archive::ZipExtractor extractor(baulk::IsQuietMode);
  if (!extractor.OpenReader(fd, dest, bela::SizeUnInitialized, offset, ec)) {
    bela::FPrintF(stderr, L"baulk extract zip error: %s\n", ec);
    return false;
  }
  if (!extractor.Extract(ec)) {
    bela::FPrintF(stderr, L"baulk extract zip error: %s\n", ec);
    return false;
  }
  return true;
}

bool extract_auto_with_mode(std::wstring_view src, std::wstring_view dest, bool bucket_mode, bela::error_code &ec) {
  using archive::file_format_t;
  file_format_t afmt{file_format_t::none};
  int64_t baseOffest = 0;
  auto fd = archive::OpenFile(src, baseOffest, afmt, ec);
  if (!fd) {
    bela::FPrintF(stderr, L"baulk extract %s error: %s\n", src, ec);
    return false;
  }
  DbgPrint(L"detect file format: %s", archive::FormatToMIME(afmt));
  switch (afmt) {
  case file_format_t::none:
    bela::FPrintF(stderr, L"unable detect %s format\n", src);
    return false;
  case file_format_t::zip:
    if (!extract_zip_simple(*fd, dest, baseOffest, ec)) {
      return false;
    }
    if (!make_flattened(dest)) {
      bela::FPrintF(stderr, L"baulk tidy %s error: %s\n", dest, ec);
    }
    return true;
  case file_format_t::xz:
    [[fallthrough]];
  case file_format_t::zstd:
    [[fallthrough]];
  case file_format_t::gz:
    [[fallthrough]];
  case file_format_t::bz2:
    [[fallthrough]];
  case file_format_t::tar:
    if (!extract_tar(*fd, baseOffest, afmt, src, dest, ec)) {
      bela::FPrintF(stderr, L"baulk extract %s error: %s\n", src, ec);
      return false;
    }
    if (!make_flattened(dest)) {
      bela::FPrintF(stderr, L"baulk tidy %s error: %s\n", dest, ec);
    }
    return true;
  case file_format_t::msi:
    fd->Assgin(INVALID_HANDLE_VALUE, false);
    if (!extract_msi(src, dest, ec)) {
      bela::FPrintF(stderr, L"baulk extract %s error: %s\n", src, ec);
      return false;
    }
    if (!make_flattened_msi(dest)) {
      bela::FPrintF(stderr, L"baulk tidy %s error: %s\n", dest, ec);
    }
    return true;
  case file_format_t::exe:
    if (bucket_mode) {
      fd->Assgin(INVALID_HANDLE_VALUE, false);
      if (!extract_exe(src, dest, ec)) {
        bela::FPrintF(stderr, L"baulk extract %s error: %s\n", src, ec);
        return false;
      }
      if (!make_flattened_exe(dest)) {
        bela::FPrintF(stderr, L"baulk tidy %s error: %s\n", dest, ec);
      }
      return true;
    }
    bela::FPrintF(stderr, L"extract pure PE/COFF not support\n");
    return false;
  default:
    break;
  }
  fd->Assgin(INVALID_HANDLE_VALUE, false);
  if (!extract_7z(src, dest, ec)) {
    bela::FPrintF(stderr, L"baulk extract %s error: %s\n", src, ec);
    return false;
  }
  if (!make_flattened(dest)) {
    bela::FPrintF(stderr, L"baulk tidy %s error: %s\n", dest, ec);
  }
  return true;
}

bool extract_auto(std::wstring_view src, std::wstring_view dest, bela::error_code &ec) {
  return extract_auto_with_mode(src, dest, true, ec);
}

} // namespace baulk