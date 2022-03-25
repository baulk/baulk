///
#include <version.hpp>
#include <bela/process.hpp>
#include <bela/str_split_narrow.hpp>
#include <bela/ascii.hpp>
#include <baulk/vfs.hpp>
#include <baulk/fs.hpp>
#include <baulk/archive/extract.hpp>
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

bool Executor::extract_file(const std::wstring_view arfile) {
  extract_dest = baulk::archive::FileDestination(arfile);
  bela::error_code ec;
  if (bela::PathExists(extract_dest)) {
    bela::fs::ForceDeleteFolders(extract_dest, ec);
  }
  baulk::archive::ZipExtractor zr(false, IsDebugMode);
  if (!zr.OpenReader(arfile, extract_dest, ec)) {
    bela::FPrintF(stderr, L"baulk extract %s error: %s\n", arfile, ec);
    return false;
  }
  if (!zr.Extract(ec)) {
    bela::FPrintF(stderr, L"baulk extract %s error: %s\n", arfile, ec);
    return false;
  }
  baulk::fs::MakeFlattened(extract_dest, ec);
  return true;
}

bool Executor::apply_baulk_files() {
  const std::filesystem::path filter_paths[] = {
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
      auto ec = bela::from_std_error_code(e, L"create_directories ");
      bela::FPrintF(stderr, L"create_directories for new path: %s\n", newPath.parent_path().wstring());
      return false;
    }
    std::filesystem::rename(source, newPath, e);
    if (e) {
      auto ec = bela::from_std_error_code(e, L"rename ");
      bela::FPrintF(stderr, L"rename for new path: %s\n", newPath.c_str());
      return false;
    }
    DbgPrint(L"move %s done\n", relativePath.c_str());
    return true;
  };

  std::filesystem::path root(extract_dest);
  std::error_code e;
  for (const auto &p : std::filesystem::recursive_directory_iterator(root, e)) {
    if (p.is_directory(e)) {
      continue;
    }
    auto relativePath = std::filesystem::relative(p.path(), root, e);
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
  if (rename_file(baulkUpdate, baulkUpdateDiscard) && rename_file(baulkUpdateSource, baulkUpdate)) {
    return true;
  }
  return false;
}

bool Executor::replace_baulk_files() {
  std::filesystem::path location(vfs::AppLocation());
  std::filesystem::path source(extract_dest);
  if (!replace_baulk_exec(location, source)) {
    return false;
  }
  return replace_baulk_update(location, source);
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