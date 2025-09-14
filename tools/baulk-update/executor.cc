///
#include <utility>
#include <version.hpp>
#include <bela/process.hpp>
#include <bela/str_split_narrow.hpp>
#include <bela/ascii.hpp>
#include <bela/terminal.hpp>
#include <baulk/vfs.hpp>
#include <baulk/fs.hpp>
#include <baulk/archive/extractor.hpp>
#include <baulk/indicators.hpp>
#include "executor.hpp"

namespace baulk {
constexpr std::wstring_view file_suffix_with_arch() {
#if defined(_M_X64)
  return L"win-x64.zip";
#elif defined(_M_ARM64)
  return L"win-arm64.zip";
#else
  return L"";
#endif
}

void terminal_size_initialize(bela::terminal::terminal_size &termsz) {
  if (bela::terminal::IsTerminal(stderr)) {
    bela::terminal::TerminalSize(stderr, termsz);
    return;
  }
  if (bela::terminal::IsCygwinTerminal(stderr)) {
    CygwinTerminalSize(termsz);
  }
}

inline void progress_show(bela::terminal::terminal_size &termsz, const std::wstring_view filename) {
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

class ZipExtractor {
public:
  ZipExtractor(bela::io::FD &&fd_, std::filesystem::path archive_file_, std::filesystem::path destination_,
               const baulk::archive::ExtractorOptions &opts)
      : fd(std::move(fd_)), extractor(opts), archive_file(std::move(archive_file_)),
        destination(std::move(destination_)) {}
  bool Extract(bela::error_code &ec);
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
  bela::FPrintF(stderr, L"Extracting \x1b[35m%v\x1b[0m ...\n", archive_file.filename());
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
  if (!baulk::IsDebugMode) {
    bela::FPrintF(stderr, L"\n");
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

void Executor::cleanup() {
  STARTUPINFOW si;
  PROCESS_INFORMATION pi;
  SecureZeroMemory(&si, sizeof(si));
  SecureZeroMemory(&pi, sizeof(pi));
  si.cb = sizeof(si);
  constexpr std::wstring_view command = L"sleep 1; rm -Force -ErrorAction SilentlyContinue baulk-update.del";
  bela::EscapeArgv ea(L"powershell", L"-Command", command);
  if (CreateProcessW(nullptr, ea.data(), nullptr, nullptr, FALSE, CREATE_NO_WINDOW | CREATE_UNICODE_ENVIRONMENT,
                     nullptr, vfs::AppLocationPath().data(), &si, &pi) != TRUE) {
    auto ec = bela::make_system_error_code();
    bela::FPrintF(stderr, L"run post script failed %s\n", ec);
    return;
  }
  CloseHandle(pi.hThread);
  CloseHandle(pi.hProcess);
}

bool Executor::extract_file(const std::filesystem::path &archive_file) {
  destination = archive_file.parent_path() / baulk::archive::PathStripExtension(archive_file.filename().native());
  std::error_code e;
  if (std::filesystem::exists(destination, e)) {
    std::filesystem::remove_all(destination, e);
  }
  bela::error_code ec;
  if (!extract_zip(archive_file, destination, ec)) {
    bela::FPrintF(stderr, L"baulk extract: %v error: %v\n", archive_file.filename(), ec);
    return false;
  }
  return true;
}

bool Executor::apply_baulk_files() {
  static const std::filesystem::path filter_paths[] = {
      L"bin/baulk-exec.exe",   // exec
      L"bin/baulk-update.exe", // update
      L"config/baulk.json",    // config file
      L"baulk.env",            // env file
  };
  auto file_filter = [&](const std::filesystem::path &p) -> bool {
    for (const auto &i : filter_paths) {
      if (i == p) {
        return true;
      }
    }
    return false;
  };
  // location baulk location
  std::filesystem::path location(vfs::AppLocation());
  // checked_move_file check and move file
  auto checked_move_file = [&](const std::filesystem::path &source, const std::filesystem::path &relativePath) -> bool {
    std::filesystem::path newPath = location / relativePath;
    std::error_code e;
    if (std::filesystem::create_directories(newPath.parent_path(), e); e) {
      auto ec = bela::make_error_code_from_std(e, L"create_directories ");
      bela::FPrintF(stderr, L"create_directories for new path: %s\n", newPath.parent_path().wstring());
      return false;
    }
    std::filesystem::rename(source, newPath, e);
    if (e) {
      auto ec = bela::make_error_code_from_std(e, L"rename ");
      bela::FPrintF(stderr, L"rename for new path: %s\n", newPath.c_str());
      return false;
    }
    DbgPrint(L"move %s done\n", relativePath.c_str());
    return true;
  };

  std::error_code e;
  for (const auto &p : std::filesystem::recursive_directory_iterator{destination, e}) {
    if (p.is_directory(e)) {
      continue;
    }
    auto relativePath = std::filesystem::relative(p.path(), destination, e);
    if (e) {
      continue;
    }
    if (file_filter(relativePath)) {
      continue;
    }
    checked_move_file(p.path(), relativePath);
  }
  return true;
}

inline bool rename_file(const std::filesystem::path &oldfile, const std::filesystem::path &newfile) {
  std::error_code e;
  std::filesystem::rename(oldfile, newfile, e);
  return !e;
}

// baulk-exec running
inline bool replace_baulk_exec(const std::filesystem::path &location, const std::filesystem::path &source) {
  auto baulkExecDiscard = location / L"bin/baulk-exec.del";
  auto baulkExecNew = location / L"bin/baulk-exec.new";
  auto baulkExec = location / L"bin/baulk-exec.exe";
  auto baulkExecSource = source / L"bin/baulk-exec.exe";
  std::error_code e;
  if (std::filesystem::exists(baulkExec, e)) {
    if (rename_file(baulkExec, baulkExecDiscard) && rename_file(baulkExecSource, baulkExec)) {
      return true;
    }
  }
  if (rename_file(baulkExecSource, baulkExec)) {
    return true;
  }
  return rename_file(baulkExecSource, baulkExecNew);
}

inline bool replace_baulk_update(const std::filesystem::path &location, const std::filesystem::path &source) {
  auto baulkUpdateDiscard = location / L"bin/baulk-update.del";
  auto baulkUpdate = location / L"bin/baulk-update.exe";
  auto baulkUpdateSource = source / L"bin/baulk-update.exe";
  return rename_file(baulkUpdate, baulkUpdateDiscard) && rename_file(baulkUpdateSource, baulkUpdate);
}

bool Executor::replace_baulk_files() {
  std::filesystem::path location(vfs::AppLocation());
  if (!replace_baulk_exec(location, destination)) {
    return false;
  }
  return replace_baulk_update(location, destination);
}

bool Executor::Execute() {
  if (!latest_download_url()) {
    return true;
  }
  auto arfile = download_file();
  if (!arfile) {
    return false;
  }
  if (!extract_file(*arfile)) {
    return false;
  }
  if (!apply_baulk_files()) {
    return false;
  }
  return replace_baulk_files();
}

} // namespace baulk