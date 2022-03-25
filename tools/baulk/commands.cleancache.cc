// pkgclean command cleanup pkg cache
#include <bela/terminal.hpp>
#include <baulk/fs.hpp>
#include <baulk/vfs.hpp>
#include "baulk.hpp"
#include "commands.hpp"

namespace baulk::commands {

bool FileIsExpired(std::wstring_view file, uint64_t ufnow) {
  FILETIME ftCreate{0};
  FILETIME ftAccess{0};
  FILETIME ftWrite{0};
  HANDLE FileHandle = CreateFileW(file.data(), 0, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr,
                                  OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr);
  if (FileHandle == INVALID_HANDLE_VALUE) {
    auto ec = bela::make_system_error_code();
    bela::FPrintF(stderr, L"CreateFileW() %s: %s\n", file, ec);
    return false;
  }
  auto closer = bela::finally([&] { CloseHandle(FileHandle); });
  // Retrieve the file times for the file.
  if (GetFileTime(FileHandle, &ftCreate, &ftAccess, &ftWrite) != TRUE) {
    auto ec = bela::make_system_error_code();
    bela::FPrintF(stderr, L"GetFileTime() %s: %s\n", file, ec);
    return false;
  }
  constexpr auto expires = 30LL * 24 * 3600 * 1000; // ms
  ULARGE_INTEGER uc;
  uc.HighPart = ftCreate.dwHighDateTime;
  uc.LowPart = ftCreate.dwLowDateTime;
  auto diffInMillis = (ufnow - uc.QuadPart) / 10000;
  return diffInMillis > expires;
}

void usage_cleancache() {
  bela::FPrintF(stderr, LR"(Usage: baulk cleancache [<args>]
Cleanup download cache

Example:
  baulk cleancache
  baulk cleancache --force

)");
}

int cmd_cleancache(const argv_t & /*unused*/) {
  bela::error_code ec;
  SYSTEMTIME now;
  GetSystemTime(&now);
  FILETIME fnow;
  if (SystemTimeToFileTime(&now, &fnow) != TRUE) {
    ec = bela::make_system_error_code();
    bela::FPrintF(stderr, L"SystemTimeToFileTime: %s\n", ec);
    return 1;
  }
  ULARGE_INTEGER ul;
  ul.LowPart = fnow.dwLowDateTime;
  ul.HighPart = fnow.dwHighDateTime;
  std::error_code e;
  for (const auto &p : std::filesystem::directory_iterator{vfs::AppTemp(), e}) {
    auto path_ = p.path();
    if (baulk::IsForceMode || p.is_directory()) {
      bela::fs::ForceDeleteFolders(path_.native(), ec);
      continue;
    }
    if (FileIsExpired(path_.native(), ul.QuadPart)) {
      DbgPrint(L"%s expired", path_.native());
      bela::fs::ForceDeleteFolders(path_.native(), ec);
      continue;
    }
  }
  return 0;
}

} // namespace baulk::commands