#include <bela/path.hpp>
#include "baulk.hpp"
#include "decompress.hpp"
#include <baulk/fs.hpp>
#include <baulk/archive/extract.hpp>

namespace baulk {

namespace standard {
bool Regularize(std::wstring_view path) {
  bela::error_code ec;
  // TODO some zip code
  return baulk::fs::MakeFlattened(path, path, ec);
}
} // namespace standard

namespace exe {
bool Decompress(std::wstring_view src, std::wstring_view dest, bela::error_code &ec) {
  if (!baulk::fs::MakeDir(dest, ec)) {
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

bool Regularize(std::wstring_view path) {
  std::error_code ec;
  for (auto &p : std::filesystem::directory_iterator(path)) {
    if (p.path().extension() == L".old") {
      std::filesystem::remove_all(p.path(), ec);
    }
  }
  return true;
}
} // namespace exe

namespace zip {

bool Decompress(std::wstring_view src, std::wstring_view outdir, bela::error_code &ec) {
  baulk::archive::ZipExtractor extractor(baulk::IsQuietMode);
  if (!extractor.OpenReader(src, outdir, ec)) {
    return false;
  }
  return extractor.Extract(ec);
}

} // namespace zip

namespace smart {
bool zip_extract(bela::io::FD &fd, std::wstring_view dest, int64_t offset, bela::error_code &ec) {
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

bool Decompress(std::wstring_view src, std::wstring_view dest, bela::error_code &ec) {
  using archive::file_format_t;
  file_format_t afmt{file_format_t::none};
  int64_t baseOffest = 0;
  auto fd = archive::OpenArchiveFile(src, baseOffest, afmt, ec);
  if (!fd) {
    bela::FPrintF(stderr, L"baulk extract %s error: %s\n", src, ec);
    return false;
  }
  DbgPrint(L"detect file format: %s", archive::FormatToMIME(afmt));
  switch (afmt) {
  case file_format_t::none:
    ec = bela::make_error_code(bela::ErrUnimplemented, L"this archive format detect unimplemented");
    return false;
  case file_format_t::zip:
    if (!zip_extract(*fd, dest, baseOffest, ec)) {
      return false;
    }
    if (!standard::Regularize(dest)) {
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
    if (!tar::TarExtract(*fd, baseOffest, afmt, src, dest, ec)) {
      bela::FPrintF(stderr, L"baulk extract %s error: %s\n", src, ec);
      return false;
    }
    if (!standard::Regularize(dest)) {
      bela::FPrintF(stderr, L"baulk tidy %s error: %s\n", dest, ec);
    }
    return true;
  case file_format_t::msi:
    fd->Assgin(INVALID_HANDLE_VALUE, false);
    if (!msi::Decompress(src, dest, ec)) {
      bela::FPrintF(stderr, L"baulk extract %s error: %s\n", src, ec);
      return false;
    }
    if (!msi::Regularize(dest)) {
      bela::FPrintF(stderr, L"baulk tidy %s error: %s\n", dest, ec);
    }
    return true;
  default:
    break;
  }
  fd->Assgin(INVALID_HANDLE_VALUE, false);
  if (!sevenzip::Decompress(src, dest, ec)) {
    bela::FPrintF(stderr, L"baulk extract %s error: %s\n", src, ec);
    return 1;
  }
  if (!standard::Regularize(dest)) {
    bela::FPrintF(stderr, L"baulk tidy %s error: %s\n", dest, ec);
  }
  return false;
}

} // namespace smart

} // namespace baulk