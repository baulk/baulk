///
#include "extractor.hpp"
#include <bela/comutils.hpp>
#include <bela/strip.hpp>
#include <bela/process.hpp>
#include <bela/simulator.hpp>
#include <bela/picker.hpp>
#include <ShObjIdl.h>
#include <ShlObj_core.h>
#include <baulk/vfs.hpp>
#include <baulk/archive.hpp>
#include <baulk/archive/msi.hpp>
#include <baulk/archive/extractor.hpp>
#include <version.hpp>

namespace baulk {
using baulk::archive::file_format_t;

class ZipExtractor final : public Extractor {
public:
  ZipExtractor(bela::io::FD &&fd_, const std::filesystem::path &archive_file_,
               const std::filesystem::path &destination_, const ExtractorOptions &opts)
      : fd(std::move(fd_)), extractor(opts), archive_file(archive_file_), destination(destination_) {}
  bool Extract(ProgressBar *bar, bela::error_code &ec);
  bool Initialize(int64_t size, int64_t offset, bela::error_code &ec) {
    return extractor.OpenReader(fd, destination, size, offset, ec);
  }

private:
  bela::io::FD fd;
  std::filesystem::path archive_file;
  std::filesystem::path destination;
  baulk::archive::zip::Extractor extractor;
};

bool ZipExtractor::Extract(ProgressBar *bar, bela::error_code &ec) {
  bar->Title(bela::StringCat(L"Extracting ", archive_file.filename()));
  bar->UpdateLine(1, destination.native(), TRUE);
  auto uncompressed_size = extractor.UncompressedSize();
  int64_t completed_bytes = 0;
  return extractor.Extract(
      [&](const baulk::archive::zip::File &file, const std::wstring &relative_name) -> bool {
        //
        bar->UpdateLine(2, relative_name, TRUE);
        return !bar->Cancelled();
      },
      [&](size_t bytes) -> bool {
        completed_bytes += bytes;
        bar->Update(completed_bytes, uncompressed_size);
        return !bar->Cancelled();
      },
      ec);
}

// tar or gz and other archive
class UniversalExtractor final : public Extractor {
public:
  UniversalExtractor(bela::io::FD &&fd_, const std::filesystem::path &archive_file_,
                     const std::filesystem::path &destination_, const ExtractorOptions &opts_, int64_t offset_,
                     file_format_t afmt_)
      : fd(std::move(fd_)), archive_file(archive_file_), destination(destination_), opts(opts_), offset(offset_),
        afmt(afmt_) {}
  bool Extract(ProgressBar *bar, bela::error_code &ec);

private:
  bool single_file_extract(ProgressBar *bar, bela::error_code &ec);
  bool tar_extract(ProgressBar *bar, bela::error_code &ec);
  bool tar_extract(ProgressBar *bar, baulk::archive::tar::FileReader &fr, baulk::archive::tar::ExtractReader *reader,
                   bela::error_code &ec);
  bela::io::FD fd;
  std::filesystem::path archive_file;
  std::filesystem::path destination;
  ExtractorOptions opts;
  int64_t offset{0};
  file_format_t afmt{file_format_t::none};
};

bool UniversalExtractor::tar_extract(ProgressBar *bar, baulk::archive::tar::FileReader &fr,
                                     baulk::archive::tar::ExtractReader *reader, bela::error_code &ec) {
  auto size = fd.Size(ec);
  if (size == bela::SizeUnInitialized) {
    return false;
  }
  baulk::archive::tar::Extractor extractor(reader, opts);
  if (!extractor.InitializeExtractor(destination, ec)) {
    return false;
  }
  bar->Title(bela::StringCat(L"Extract ", archive_file.filename()));
  bar->UpdateLine(1, destination.native(), TRUE);
  return extractor.Extract(
      [&](const baulk::archive::tar::Header &hdr, const std::wstring &relative_name) -> bool {
        bar->UpdateLine(2, relative_name, TRUE);
        bar->Update(fr.Position(), size);
        return !bar->Cancelled();
      },
      nullptr, ec);
}

bool UniversalExtractor::tar_extract(ProgressBar *bar, bela::error_code &ec) {
  baulk::archive::tar::FileReader fr(fd.NativeFD());
  if (auto wr = baulk::archive::tar::MakeReader(fr, offset, afmt, ec); wr) {
    return tar_extract(bar, fr, wr.get(), ec);
  }
  if (ec != baulk::archive::tar::ErrNoFilter) {
    return false;
  }
  return tar_extract(bar, fr, &fr, ec);
}

bool UniversalExtractor::single_file_extract(ProgressBar *bar, bela::error_code &ec) {
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
  bar->Title(bela::StringCat(L"Extracting ", archive_file.filename()));
  bar->UpdateLine(1, destination.native(), TRUE);
  bar->UpdateLine(2, filename.native(), TRUE);
  uint8_t buffer[8192];
  for (;;) {
    auto nBytes = wr->Read(buffer, sizeof(buffer), ec);
    if (nBytes <= 0) {
      break;
    }
    bar->Update(fr.Position(), size);
    if (!fd->WriteFull(buffer, static_cast<size_t>(nBytes), ec)) {
      return false;
    }
  }
  return true;
}

bool UniversalExtractor::Extract(ProgressBar *bar, bela::error_code &ec) {
  if (tar_extract(bar, ec)) {
    return true;
  }
  if (ec != baulk::archive::ErrAnotherWay) {
    return false;
  }
  return single_file_extract(bar, ec);
}

class MsiExtractor final : public Extractor {
public:
  MsiExtractor(const std::filesystem::path &archive_file_, const std::filesystem::path &destination_)
      : archive_file(archive_file_), destination(destination_) {}
  bool Extract(ProgressBar *bar, bela::error_code &ec);

private:
  std::filesystem::path archive_file;
  std::filesystem::path destination;
};

bool MsiExtractor::Extract(ProgressBar *bar, bela::error_code &ec) {
  baulk::archive::msi::Extractor extractor;
  if (!extractor.Initialize(
          archive_file, destination,
          [&](baulk::archive::msi::Dispatcher &d) {
            d.Assign(
                [&](int64_t extracted, int64_t total) -> bool {
                  bar->Update(extracted, total);
                  return !bar->Cancelled();
                },
                [&](std::wstring_view message, baulk::archive::msi::MessageLevel level) {
                  switch (level) {
                  case baulk::archive::msi::MessageFatal:
                    bela::BelaMessageBox(nullptr, L"Extract Fatal", message.data(), BAULK_APPLINK, bela::mbs_t::FATAL);
                    break;
                  case baulk::archive::msi::MessageError:
                    bela::BelaMessageBox(nullptr, L"Extract Error", message.data(), BAULK_APPLINK, bela::mbs_t::FATAL);
                    break;
                  case baulk::archive::msi::MessageWarn:
                    bela::BelaMessageBox(nullptr, L"Extract Warn", message.data(), BAULK_APPLINK, bela::mbs_t::WARN);
                    break;
                  default:
                    break;
                  }
                });
          },
          ec)) {
    return false;
  }
  bar->Title(bela::StringCat(L"Extracting ", archive_file.filename()));
  bar->UpdateLine(1, destination.native(), TRUE);
  if (!extractor.Extract(ec)) {
    return false;
  }
  return extractor.MakeFlattened(ec);
}

inline std::optional<std::wstring> find7zInstallPath(bela::error_code &ec) {
  HKEY hkey = nullptr;
  if (RegOpenKeyW(HKEY_LOCAL_MACHINE, LR"(SOFTWARE\7-Zip-Zstandard)", &hkey) != ERROR_SUCCESS) {
    if (RegOpenKeyW(HKEY_LOCAL_MACHINE, LR"(SOFTWARE\7-Zip)", &hkey) != ERROR_SUCCESS) {
      ec = bela::make_system_error_code();
      return std::nullopt;
    }
  }
  auto closer = bela::finally([&] { RegCloseKey(hkey); });
  wchar_t buffer[4096];
  DWORD type = 0;
  DWORD bufsize = sizeof(buffer);
  if (RegQueryValueExW(hkey, L"Path64", nullptr, &type, reinterpret_cast<LPBYTE>(buffer), &bufsize) != ERROR_SUCCESS &&
      type == REG_SZ) {
    if (auto s7z = bela::StringCat(bela::StripSuffix(buffer, L"\\"), L"\\7zG.exe"); bela::PathExists(s7z)) {
      return std::make_optional(std::move(s7z));
    }
  }
  if (RegQueryValueExW(hkey, L"Path", nullptr, &type, reinterpret_cast<LPBYTE>(buffer), &bufsize) != ERROR_SUCCESS) {
    ec = bela::make_system_error_code();
    return std::nullopt;
  }
  if (type != REG_SZ) {
    ec = bela::make_error_code(bela::ErrGeneral, L"reg key Path not REG_SZ: ", type);
    return std::nullopt;
  }
  if (auto s7z = bela::StringCat(bela::StripSuffix(buffer, L"\\"), L"\\7zG.exe"); bela::PathExists(s7z)) {
    return std::make_optional(std::move(s7z));
  }
  ec = bela::make_error_code(ERROR_NOT_FOUND, L"7zG.exe not found");
  return std::nullopt;
}

inline std::optional<std::wstring> lookup_sevenzip() {
  bela::error_code ec;
  baulk::vfs::InitializeFastPathFs(ec);
  if (auto s7z = bela::StringCat(baulk::vfs::AppLinks(), L"\\baulk7zG.exe"); bela::PathExists(s7z)) {
    return std::make_optional(std::move(s7z));
  }
  if (auto ps7z = find7zInstallPath(ec); ps7z) {
    return std::make_optional(std::move(*ps7z));
  }
  if (auto s7z = bela::StringCat(baulk::vfs::AppLinks(), L"\\7zG.exe"); !bela::PathExists(s7z)) {
    return std::make_optional(std::move(s7z));
  }
  std::wstring s7z;
  if (bela::env::LookPath(L"7zG.exe", s7z, true)) {
    return std::make_optional(std::move(s7z));
  }
  return std::nullopt;
}

class _7zExtractor final : public Extractor {
public:
  _7zExtractor(const std::filesystem::path &archive_file_, const std::filesystem::path &destination_,
               file_format_t afmt_)
      : archive_file(archive_file_), destination(destination_), afmt(afmt_) {}
  bool Extract(ProgressBar *bar, bela::error_code &ec) {
    auto _7z = lookup_sevenzip();
    if (!_7z) {
      ec = bela::make_error_code(ERROR_NOT_FOUND, L"7zG.exe not found");
      return false;
    }
    bela::process::Process process;
    if (process.Execute(*_7z, L"e", L"-spf", L"-y", archive_file.native(),
                        bela::StringCat(L"-o", destination.native())) != 0) {
      ec = process.ErrorCode();
      return false;
    }
    if (afmt == file_format_t::nsis) {
      std::error_code e;
      std::filesystem::remove_all(destination / L"$PLUGINSDIR", e);
      std::filesystem::remove_all(destination / L"Uninstall.exe", e);
    }
    return true;
  }

private:
  std::filesystem::path archive_file;
  std::filesystem::path destination;
  file_format_t afmt;
};

std::shared_ptr<Extractor> MakeExtractor(const std::filesystem::path &archive_file,
                                         const std::filesystem::path &destination, const ExtractorOptions &opts,
                                         bela::error_code &ec) {
  int64_t baseOffset = 0;
  file_format_t afmt{file_format_t::none};
  auto fd = baulk::archive::OpenFile(archive_file.native(), baseOffset, afmt, ec);
  if (!fd) {
    return nullptr;
  }
  switch (afmt) {
  case file_format_t::none:
    ec = bela::make_error_code(bela::ErrGeneral, L"unable to detect format '", archive_file.filename(), L"'");
    return nullptr;
  case file_format_t::zip: {
    auto e = std::make_shared<ZipExtractor>(std::move(*fd), archive_file, destination, opts);
    if (!e->Initialize(bela::SizeUnInitialized, baseOffset, ec)) {
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
    return std::make_shared<UniversalExtractor>(std::move(*fd), archive_file, destination, opts, baseOffset, afmt);
  case file_format_t::msi:
    fd->Assgin(INVALID_HANDLE_VALUE, false);
    return std::make_shared<MsiExtractor>(archive_file, destination);
  case file_format_t::cab:
    [[fallthrough]];
  case file_format_t::deb:
    [[fallthrough]];
  case file_format_t::dmg:
    [[fallthrough]];
  case file_format_t::rpm:
    [[fallthrough]];
  case file_format_t::wim:
    [[fallthrough]];
  case file_format_t::rar:
    [[fallthrough]];
  case file_format_t::nsis:
    [[fallthrough]];
  case file_format_t::_7z:
    fd->Assgin(INVALID_HANDLE_VALUE, false);
    return std::make_shared<_7zExtractor>(archive_file, destination, afmt);
  default:
    break;
  }
  ec = bela::make_error_code(L"file format not support");
  return nullptr;
}
} // namespace baulk