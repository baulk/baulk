///
#include "extractor.hpp"
#include <bela/comutils.hpp>
#include <bela/strip.hpp>
#include <bela/process.hpp>
#include <bela/simulator.hpp>
#include <bela/terminal.hpp>
#include <ShObjIdl.h>
#include <ShlObj_core.h>
#include <baulk/vfs.hpp>
#include <baulk/archive.hpp>
#include <baulk/archive/msi.hpp>
#include <baulk/archive/extractor.hpp>
#include <baulk/archive/7zfinder.hpp>
#include <baulk/indicators.hpp>
#include <baulk/debug.hpp>
#include <utility>
#include "baulk.hpp"

namespace baulk {

void terminal_size_initialize(bela::terminal::terminal_size &termsz) {
  if (baulk::IsQuietMode) {
    return;
  }
  if (bela::terminal::IsTerminal(stderr)) {
    bela::terminal::TerminalSize(stderr, termsz);
    return;
  }
  if (bela::terminal::IsCygwinTerminal(stderr)) {
    CygwinTerminalSize(termsz);
  }
}

inline void progress_show(bela::terminal::terminal_size &termsz, const std::wstring_view filename) {
  if (baulk::IsQuietMode) {
    return;
  }
  if (baulk::IsDebugMode) {
    bela::FPrintF(stderr, L"\x1b[33m* x %s\x1b[0m\n", filename);
    return;
  }
  auto suglen = static_cast<size_t>(termsz.columns) - 8;
  if (auto n = bela::string_width<wchar_t>(filename); n <= suglen) {
    bela::FPrintF(stderr, L"\x1b[2K\r\x1b[33mx %s\x1b[0m", filename);
    return;
  }
  bela::FPrintF(stderr, L"\x1b[2K\r\x1b[33mx ...\\%s\x1b[0m", bela::BaseName(filename));
}

class ZipExtractor final : public Extractor {
public:
  ZipExtractor(bela::io::FD &&fd_, std::filesystem::path archive_file_, std::filesystem::path destination_,
               const ExtractorOptions &opts)
      : fd(std::move(fd_)), extractor(opts), archive_file(std::move(archive_file_)),
        destination(std::move(destination_)) {}
  bool Extract(bela::error_code &ec) override;
  bool Initialize(int64_t size, int64_t offset, bela::error_code &ec) {
    return extractor.OpenReader(fd, destination, size, offset, ec);
  }

private:
  bela::io::FD fd;
  std::filesystem::path archive_file;
  std::filesystem::path destination;
  baulk::archive::zip::Extractor extractor;
};

bool ZipExtractor::Extract(bela::error_code &ec) {
  bela::FPrintF(stderr, L"Extracting \x1b[36m%v\x1b[0m ...\n", archive_file.filename());
  bela::terminal::terminal_size termsz;
  terminal_size_initialize(termsz);
  auto uncompressed_size = extractor.UncompressedSize();
  int64_t completed_bytes = 0;
  if (!extractor.Extract(
          [&](const baulk::archive::zip::File &file, const std::wstring &relative_name) -> bool {
            progress_show(termsz, relative_name);
            return true;
          },
          nullptr, ec)) {

    return false;
  }
  if (!baulk::IsDebugMode && !baulk::IsQuietMode) {
    bela::FPrintF(stderr, L"\n");
  }
  return true;
}

// tar or gz and other archive
class UniversalExtractor final : public Extractor {
public:
  UniversalExtractor(bela::io::FD &&fd_, std::filesystem::path archive_file_, std::filesystem::path destination_,
                     const ExtractorOptions &opts_, int64_t offset_, baulk::archive::file_format_t afmt_)
      : fd(std::move(fd_)), archive_file(std::move(archive_file_)), destination(std::move(destination_)), opts(opts_),
        offset(offset_), afmt(afmt_) {}
  bool Extract(bela::error_code &ec) override;

private:
  bool single_file_extract(bela::error_code &ec);
  bool tar_extract(bela::error_code &ec);
  bool tar_extract(baulk::archive::tar::FileReader &fr, baulk::archive::tar::ExtractReader *reader,
                   bela::error_code &ec);
  bela::io::FD fd;
  std::filesystem::path archive_file;
  std::filesystem::path destination;
  ExtractorOptions opts;
  int64_t offset{0};
  baulk::archive::file_format_t afmt;
};

bool UniversalExtractor::tar_extract(baulk::archive::tar::FileReader &fr, baulk::archive::tar::ExtractReader *reader,
                                     bela::error_code &ec) {
  if (auto size = fd.Size(ec); size == bela::SizeUnInitialized) {
    return false;
  }
  baulk::archive::tar::Extractor extractor(reader, opts);
  if (!extractor.InitializeExtractor(destination, ec)) {
    return false;
  }
  bela::terminal::terminal_size termsz;
  terminal_size_initialize(termsz);
  if (!extractor.Extract(
          [&](const baulk::archive::tar::Header &hdr, const std::wstring &relative_name) -> bool {
            progress_show(termsz, relative_name);
            return true;
          },
          nullptr, ec)) {
    return false;
  }
  if (!baulk::IsDebugMode && !baulk::IsQuietMode) {
    bela::FPrintF(stderr, L"\n");
  }
  return true;
}

bool UniversalExtractor::tar_extract(bela::error_code &ec) {
  baulk::archive::tar::FileReader fr(fd.NativeFD());
  if (auto wr = baulk::archive::tar::MakeReader(fr, offset, afmt, ec); wr) {
    return tar_extract(fr, wr.get(), ec);
  }
  if (ec != baulk::archive::tar::ErrNoFilter) {
    return false;
  }
  return tar_extract(fr, &fr, ec);
}

bool UniversalExtractor::single_file_extract(bela::error_code &ec) {
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
  filename.replace_extension();
  auto target = destination / filename;
  auto fd = baulk::archive::File::NewFile(target, bela::Now(), opts.overwrite_mode, ec);
  if (!fd) {
    return false;
  }
  baulk::ProgressBar bar;
  bar.FileName(bela::StringCat(L"Extracting ", archive_file.filename()));
  bar.Maximum(static_cast<uint64_t>(size));
  if (!baulk::IsQuietMode) {
    bar.Execute();
  }
  // defer close bar
  auto close_bar = bela::finally([&] { bar.Finish(); });
  int64_t old_total = 0;
  uint8_t buffer[8192];
  for (;;) {
    auto nBytes = wr->Read(buffer, sizeof(buffer), ec);
    if (nBytes <= 0) {
      break;
    }
    bar.Update(static_cast<uint64_t>(fr.Position()));
    // bar->Update(fr.Position(), size);
    if (!fd->WriteFull(buffer, static_cast<size_t>(nBytes), ec)) {
      bar.MarkFault();
      bar.MarkCompleted();
      return false;
    }
  }
  bar.MarkCompleted();
  return true;
}

bool UniversalExtractor::Extract(bela::error_code &ec) {
  bela::FPrintF(stderr, L"Extracting \x1b[36m%v\x1b[0m ...\n", archive_file.filename());
  if (tar_extract(ec)) {
    return true;
  }
  if (ec != baulk::archive::ErrAnotherWay) {
    return false;
  }
  return single_file_extract(ec);
}

class MsiExtractor final : public Extractor {
public:
  MsiExtractor(std::filesystem::path archive_file_, std::filesystem::path destination_)
      : archive_file(std::move(archive_file_)), destination(std::move(destination_)) {}
  bool Extract(bela::error_code &ec) override;

private:
  std::filesystem::path archive_file;
  std::filesystem::path destination;
};

bool MsiExtractor::Extract(bela::error_code &ec) {
  bela::FPrintF(stderr, L"Extracting \x1b[36m%v\x1b[0m ...\n", archive_file.filename());
  baulk::archive::msi::Extractor extractor;
  baulk::ProgressBar bar;
  bar.FileName(bela::StringCat(L"Extracting ", archive_file.filename()));
  if (!baulk::IsQuietMode) {
    bar.Execute();
  }
  // defer close bar
  auto close_bar = bela::finally([&] { bar.Finish(); });
  int64_t old_total = 0;
  if (!extractor.Initialize(
          archive_file, destination,
          [&](baulk::archive::msi::Dispatcher &d) {
            d.Assign(
                [&](int64_t extracted, int64_t total) -> bool {
                  if (total != old_total) {
                    if (total != 0) {
                      bar.Maximum(static_cast<uint64_t>(total));
                    }
                    old_total = total;
                  }
                  if (extracted != 0) {
                    bar.Update(static_cast<uint64_t>(extracted));
                  }
                  return true;
                },
                [&](std::wstring_view message, baulk::archive::msi::MessageLevel level) {
                  switch (level) {
                  case baulk::archive::msi::MessageFatal:
                    ec = bela::make_error_code(bela::ErrGeneral, L"msi extract fatal: ", message);
                    break;
                  case baulk::archive::msi::MessageError:
                    ec = bela::make_error_code(bela::ErrGeneral, L"msi extract error: ", message);
                    break;
                  case baulk::archive::msi::MessageWarn:
                    bela::FPrintF(stderr, L"\nextract msi: warning: %v\n", message);
                    break;
                  default:
                    break;
                  }
                });
          },
          ec)) {
    bar.MarkFault();
    bar.MarkCompleted();
    return false;
  }
  if (!extractor.Extract(ec)) {
    bar.MarkFault();
    bar.MarkCompleted();
    return false;
  }
  bar.MarkCompleted();
  return extractor.MakeFlattened(ec);
}

inline std::optional<std::wstring> lookup_sevenzip() {
  bela::error_code ec;
  baulk::vfs::InitializeFastPathFs(ec);
  if (auto ps7z = baulk::archive::Find7z(L"7z.exe", ec); ps7z) {
    return std::make_optional(std::move(*ps7z));
  }
  if (auto s7z = bela::StringCat(baulk::vfs::AppLinks(), L"\\7z.exe"); !bela::PathExists(s7z)) {
    return std::make_optional(std::move(s7z));
  }
  std::wstring s7z;
  if (bela::env::LookPath(L"7z.exe", s7z, true)) {
    return std::make_optional(std::move(s7z));
  }
  return std::nullopt;
}

class _7zExtractor final : public Extractor {
public:
  _7zExtractor(std::filesystem::path archive_file_, std::filesystem::path destination_,
               baulk::archive::file_format_t afmt_)
      : archive_file(std::move(archive_file_)), destination(std::move(destination_)), afmt(afmt_) {}
  bool Extract(bela::error_code &ec) override {
    bela::FPrintF(stderr, L"Extracting \x1b[36m%v\x1b[0m ...\n", archive_file.filename());
    auto _7z = lookup_sevenzip();
    if (!_7z) {
      ec = bela::make_error_code(ERROR_NOT_FOUND, L"7z.exe not found");
      return false;
    }
    bela::process::Process process;
    if (process.Execute(*_7z, L"e", L"-spf", L"-y", archive_file.native(),
                        bela::StringCat(L"-o", destination.native())) != 0) {
      ec = process.ErrorCode();
      return false;
    }
    if (afmt == baulk::archive::file_format_t::nsis) {
      std::error_code e;
      std::filesystem::remove_all(destination / L"$PLUGINSDIR", e);
      std::filesystem::remove_all(destination / L"Uninstall.exe", e);
    }
    return true;
  }

private:
  std::filesystem::path archive_file;
  std::filesystem::path destination;
  baulk::archive::file_format_t afmt;
};

std::shared_ptr<Extractor> MakeExtractor(const std::filesystem::path &archive_file,
                                         const std::filesystem::path &destination, const ExtractorOptions &opts,
                                         bela::error_code &ec) {
  int64_t baseOffset = 0;
  baulk::archive::file_format_t afmt{};
  auto fd = baulk::archive::OpenFile(archive_file.native(), baseOffset, afmt, ec);
  if (!fd) {
    return nullptr;
  }
  DbgPrint(L"extract archive: %v format: %v", archive_file, bela::integral_cast(afmt));
  switch (afmt) {
  case baulk::archive::file_format_t::none:
    ec = bela::make_error_code(bela::ErrGeneral, L"unable to detect format '", archive_file.filename(), L"'");
    return nullptr;
  case baulk::archive::file_format_t::zip: {
    auto e = std::make_shared<ZipExtractor>(std::move(*fd), archive_file, destination, opts);
    if (!e->Initialize(bela::SizeUnInitialized, baseOffset, ec)) {
      return nullptr;
    }
    return e;
  }
  case baulk::archive::file_format_t::xz:
    [[fallthrough]];
  case baulk::archive::file_format_t::zstd:
    [[fallthrough]];
  case baulk::archive::file_format_t::gz:
    [[fallthrough]];
  case baulk::archive::file_format_t::bz2:
    [[fallthrough]];
  case baulk::archive::file_format_t::tar:
    return std::make_shared<UniversalExtractor>(std::move(*fd), archive_file, destination, opts, baseOffset, afmt);
  case baulk::archive::file_format_t::cab:
    [[fallthrough]];
  case baulk::archive::file_format_t::deb:
    [[fallthrough]];
  case baulk::archive::file_format_t::dmg:
    [[fallthrough]];
  case baulk::archive::file_format_t::rpm:
    [[fallthrough]];
  case baulk::archive::file_format_t::wim:
    [[fallthrough]];
  case baulk::archive::file_format_t::rar:
    [[fallthrough]];
  case baulk::archive::file_format_t::nsis:
    [[fallthrough]];
  case baulk::archive::file_format_t::_7z:
    fd->Assgin(INVALID_HANDLE_VALUE, false);
    return std::make_shared<_7zExtractor>(archive_file, destination, afmt);
  case baulk::archive::file_format_t::msi:
    fd->Assgin(INVALID_HANDLE_VALUE, false);
    return std::make_shared<MsiExtractor>(archive_file, destination);
  case baulk::archive::file_format_t::exe:
    ec = bela::make_error_code(baulk::archive::ErrNoOverlayArchive, L"no overlay archive");
    return nullptr;
  default:
    break;
  }
  ec = bela::make_error_code(L"file format not support");
  return nullptr;
}

bool extract_exe(const std::filesystem::path &archive_file, const std::filesystem::path &destination,
                 bela::error_code &ec) {
  auto newTarget = destination / archive_file.filename();
  std::error_code e;
  std::filesystem::remove_all(destination, e);
  std::filesystem::create_directories(destination, e);
  if (std::filesystem::rename(archive_file, newTarget, e); e) {
    ec = e;
    return false;
  }
  return true;
}

bool extract_msi(const std::filesystem::path &archive_file, const std::filesystem::path &destination,
                 bela::error_code &ec) {
  MsiExtractor extractor(archive_file, destination);
  if (!extractor.Extract(ec)) {
    baulk::DbgPrint(L"extract msi archive: %v error %v", archive_file.filename(), ec);
    return false;
  }
  return true;
}

bool extract_zip(const std::filesystem::path &archive_file, const std::filesystem::path &destination,
                 bela::error_code &ec) {
  baulk::archive::file_format_t afmt{};
  int64_t baseOffset = 0;
  auto fd = archive::OpenFile(archive_file.native(), baseOffset, afmt, ec);
  if (!fd) {
    bela::FPrintF(stderr, L"baulk open archive %s error: %s\n", archive_file.filename(), ec);
    return false;
  }
  if (afmt != baulk::archive::file_format_t::zip) {
    bela::FPrintF(stderr, L"baulk unzip %s terminated. file format: %s\n", archive_file.filename(),
                  baulk::archive::FormatToMIME(afmt));
    return false;
  }
  ZipExtractor extractor(std::move(*fd), archive_file, destination, baulk::archive::ExtractorOptions{});
  if (!extractor.Initialize(bela::SizeUnInitialized, baseOffset, ec)) {
    return false;
  }
  if (!extractor.Extract(ec)) {
    return false;
  }
  return baulk::fs::MakeFlattened(destination, ec);
}

bool extract_7z(const std::filesystem::path &archive_file, const std::filesystem::path &destination,
                bela::error_code &ec) {
  baulk::archive::file_format_t afmt{};
  int64_t baseOffset = 0;
  auto fd = archive::OpenFile(archive_file.native(), baseOffset, afmt, ec);
  if (!fd) {
    bela::FPrintF(stderr, L"baulk open archive %s error: %s\n", archive_file.filename(), ec);
    return false;
  }
  fd->Assgin(INVALID_HANDLE_VALUE, false);
  _7zExtractor extractor(archive_file, destination, afmt);
  if (!extractor.Extract(ec)) {
    return false;
  }
  return baulk::fs::MakeFlattened(destination, ec);
}

bool extract_tar(const std::filesystem::path &archive_file, const std::filesystem::path &destination,
                 bela::error_code &ec) {
  baulk::archive::file_format_t afmt{};
  int64_t baseOffset = 0;
  auto fd = archive::OpenFile(archive_file.native(), baseOffset, afmt, ec);
  if (!fd) {
    bela::FPrintF(stderr, L"baulk open archive %s error: %s\n", archive_file.filename(), ec);
    return false;
  }
  UniversalExtractor extractor(std::move(*fd), archive_file, destination, baulk::archive::ExtractorOptions{},
                               baseOffset, afmt);
  if (!extractor.Extract(ec)) {
    return false;
  }
  return baulk::fs::MakeFlattened(destination, ec);
}

bool extract_auto(const std::filesystem::path &archive_file, const std::filesystem::path &destination,
                  bela::error_code &ec) {
  auto extractor = MakeExtractor(archive_file, destination, baulk::archive::ExtractorOptions{}, ec);
  if (!extractor) {
    return false;
  }
  if (ec == baulk::archive::ErrNoOverlayArchive) {
    return extract_exe(archive_file, destination, ec);
  }
  if (!extractor->Extract(ec)) {
    return false;
  }
  return baulk::fs::MakeFlattened(destination, ec);
}

bool extract_command_auto(const std::filesystem::path &archive_file, const std::filesystem::path &destination,
                          bela::error_code &ec) {
  auto extractor = MakeExtractor(archive_file, destination, baulk::archive::ExtractorOptions{}, ec);
  if (!extractor) {
    return false;
  }
  if (!extractor->Extract(ec)) {
    return false;
  }
  return baulk::fs::MakeFlattened(destination, ec);
}

std::optional<std::filesystem::path> make_unqiue_extracted_destination(const std::filesystem::path &archive_file,
                                                                       std::filesystem::path &strict_folder) {
  std::error_code e;
  auto parent_path = archive_file.parent_path();
  strict_folder = baulk::archive::PathStripExtension(archive_file.filename().native());
  auto d = parent_path / strict_folder;
  if (!std::filesystem::exists(d, e)) {
    return std::make_optional(std::move(d));
  }
  for (int i = 1; i < 100; i++) {
    d = parent_path / bela::StringCat(strict_folder, L"-(", i, L")");
    if (!std::filesystem::exists(d, e)) {
      return std::make_optional(std::move(d));
    }
  }
  return std::nullopt;
}

} // namespace baulk