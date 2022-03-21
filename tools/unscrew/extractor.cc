///
#include "extractor.hpp"
#include <bela/comutils.hpp>
#include <ShObjIdl.h>
#include <ShlObj_core.h>
#include <baulk/archive.hpp>
#include <baulk/archive/extractor.hpp>

namespace baulk {
using baulk::archive::file_format_t;

class ZipExtractor final : public Extractor {
public:
  ZipExtractor(bela::io::FD &&fd_, const fs::path &archive_file_, const ExtractorOptions &opts)
      : fd(std::move(fd_)), extractor(opts), archive_file(archive_file_) {}
  bool Extract(IProgressDialog *bar, bela::error_code &ec);
  bool Initialize(const fs::path &dest, int64_t size, int64_t offset, bela::error_code &ec) {
    return extractor.OpenReader(fd, dest, size, offset, ec);
  }

private:
  bela::io::FD fd;
  fs::path archive_file;
  baulk::archive::zip::Extractor extractor;
};

bool ZipExtractor::Extract(IProgressDialog *bar, bela::error_code &ec) {
  bar->SetTitle(bela::StringCat(L"Extract ", archive_file.filename().native()).data());
  if (bar->StartProgressDialog(nullptr, nullptr, PROGDLG_AUTOTIME, nullptr) != S_OK) {
    auto ec = bela::make_system_error_code(L"StartProgressDialog ");
    return false;
  }
  auto uncompressed_size = extractor.UncompressedSize();
  int64_t completed_bytes = 0;
  return extractor.Extract(
      [&](const baulk::archive::zip::File &file, const std::wstring &relative_name) -> bool {
        //
        bar->SetLine(2, relative_name.data(), TRUE, nullptr);
        return bar->HasUserCancelled() != TRUE;
      },
      [&](size_t bytes) -> bool {
        completed_bytes += bytes;
        bar->SetProgress64(completed_bytes, uncompressed_size);
        return bar->HasUserCancelled() != TRUE;
      },
      ec);
}

// tar or gz and other archive
class UniversalExtractor final : public Extractor {
public:
  UniversalExtractor(bela::io::FD &&fd_, const fs::path &archive_file_, const fs::path &dest_,
                     const ExtractorOptions &opts_, int64_t offset_, file_format_t afmt_)
      : fd(std::move(fd_)), archive_file(archive_file_), dest(dest_), opts(opts_), offset(offset_), afmt(afmt_) {}
  bool Extract(IProgressDialog *bar, bela::error_code &ec);

private:
  bool single_file_extract(IProgressDialog *bar, bela::error_code &ec);
  bool tar_extract(IProgressDialog *bar, bela::error_code &ec);
  bool tar_extract(IProgressDialog *bar, baulk::archive::tar::FileReader &fr,
                   baulk::archive::tar::ExtractReader *reader, bela::error_code &ec);
  bela::io::FD fd;
  fs::path archive_file;
  fs::path dest;
  ExtractorOptions opts;
  int64_t offset{0};
  file_format_t afmt{file_format_t::none};
};

bool UniversalExtractor::tar_extract(IProgressDialog *bar, baulk::archive::tar::FileReader &fr,
                                     baulk::archive::tar::ExtractReader *reader, bela::error_code &ec) {
  auto size = fd.Size(ec);
  if (size == bela::SizeUnInitialized) {
    return false;
  }
  baulk::archive::tar::Extractor extractor(reader, opts);
  if (!extractor.InitializeExtractor(dest, ec)) {
    return false;
  }
  bar->SetTitle(bela::StringCat(L"Extract ", archive_file.filename().native()).data());
  if (bar->StartProgressDialog(nullptr, nullptr, PROGDLG_AUTOTIME, nullptr) != S_OK) {
    auto ec = bela::make_system_error_code(L"StartProgressDialog ");
    return false;
  }
  return extractor.Extract(
      [&](const baulk::archive::tar::Header &hdr, const std::wstring &relative_name) -> bool {
        bar->SetLine(2, relative_name.data(), TRUE, nullptr);
        bar->SetProgress64(fr.Position(), size);
        return bar->HasUserCancelled() != TRUE;
      },
      nullptr, ec);
}

bool UniversalExtractor::tar_extract(IProgressDialog *bar, bela::error_code &ec) {
  baulk::archive::tar::FileReader fr(fd.NativeFD());
  if (auto wr = baulk::archive::tar::MakeReader(fr, offset, afmt, ec); wr) {
    return tar_extract(bar, fr, wr.get(), ec);
  }
  if (ec.code != baulk::archive::tar::ErrNoFilter) {
    return false;
  }
  return tar_extract(bar, fr, &fr, ec);
}

bool UniversalExtractor::single_file_extract(IProgressDialog *bar, bela::error_code &ec) {
  auto size = fd.Size(ec);
  if (size == bela::SizeUnInitialized) {
    return false;
  }
  baulk::archive::tar::FileReader fr(fd.NativeFD());
  auto wr = baulk::archive::tar::MakeReader(fr, offset, afmt, ec);
  if (!wr) {
    return false;
  }
  auto filename = archive_file.filename();
  filename.replace_extension(L"");
  auto target = dest / filename;
  auto fd = baulk::archive::File::NewFile(target, bela::Now(), opts.overwrite_mode, ec);
  if (!fd) {
    return false;
  }
  bar->SetTitle(bela::StringCat(L"Extract ", archive_file.filename().native()).data());
  bar->SetLine(2, filename.c_str(), FALSE, nullptr);
  uint8_t buffer[8192];
  for (;;) {
    auto nBytes = wr->Read(buffer, sizeof(buffer), ec);
    if (nBytes <= 0) {
      break;
    }
    bar->SetProgress64(fr.Position(), size);
    if (!fd->WriteFull(buffer, static_cast<size_t>(nBytes), ec)) {
      return false;
    }
  }
  return true;
}

bool UniversalExtractor::Extract(IProgressDialog *bar, bela::error_code &ec) {
  if (tar_extract(bar, ec)) {
    return true;
  }
  if (ec.code != baulk::archive::ErrAnotherWay) {
    return false;
  }
  return single_file_extract(bar, ec);
}

std::shared_ptr<Extractor> MakeExtractor(const fs::path &archive_file, const fs::path &dest,
                                         const ExtractorOptions &opts, bela::error_code &ec) {
  int64_t baseOffset = 0;
  file_format_t afmt{file_format_t::none};
  auto fd = baulk::archive::OpenFile(archive_file.native(), baseOffset, afmt, ec);
  if (!fd) {
    return nullptr;
  }
  switch (afmt) {
  case file_format_t::none:
    ec = bela::make_error_code(bela::ErrGeneral, L"unable to detect format '", archive_file.filename().native(), L"'");
    return nullptr;
  case file_format_t::zip: {
    auto e = std::make_shared<ZipExtractor>(std::move(*fd), archive_file, opts);
    if (!e->Initialize(dest, bela::SizeUnInitialized, baseOffset, ec)) {
      return nullptr;
    }
    return e;
  }
  case file_format_t::xz:
    [[fallthrough]];
  case file_format_t::zstd:
    [[fallthrough]];
  case file_format_t::gz:
    [[fallthrough]];
  case file_format_t::bz2:
    [[fallthrough]];
  case file_format_t::tar:
    return std::make_shared<UniversalExtractor>(std::move(*fd), archive_file, dest, opts, baseOffset, afmt);
  default:
    break;
  }
  ec = bela::make_error_code(L"file format not support");
  return nullptr;
}
} // namespace baulk