// pkgclean command cleanup pkg cache
#include <bela/terminal.hpp>
#include "baulk.hpp"
#include "commands.hpp"
#include <baulk/fs.hpp>
#include "launcher.hpp"

namespace baulk::commands {

bool FileIsExpired(std::wstring_view file, uint64_t ufnow) {
  FILETIME ftCreate, ftAccess, ftWrite;
  auto FileHandle = CreateFileW(file.data(), 0, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr,
                                OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr);
  if (FileHandle == INVALID_HANDLE_VALUE) {
    auto ec = bela::make_system_error_code();
    bela::FPrintF(stderr, L"CreateFileW() %s: %s\n", file, ec.message);
    return false;
  }
  auto closer = bela::finally([&] { CloseHandle(FileHandle); });
  // Retrieve the file times for the file.
  if (GetFileTime(FileHandle, &ftCreate, &ftAccess, &ftWrite) != TRUE) {
    auto ec = bela::make_system_error_code();
    bela::FPrintF(stderr, L"GetFileTime() %s: %s\n", file, ec.message);
    return false;
  }
  constexpr auto expires = 30 * 24 * 3600 * 1000LL; // ms
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

int cmd_cleancache(const argv_t &argv) {
  auto pkgtemp = bela::StringCat(baulk::BaulkRoot(), L"\\", baulk::BaulkPkgTmpDir);
  bela::error_code ec;
  SYSTEMTIME now;
  GetSystemTime(&now);
  FILETIME fnow;
  if (SystemTimeToFileTime(&now, &fnow) != TRUE) {
    ec = bela::make_system_error_code();
    bela::FPrintF(stderr, L"SystemTimeToFileTime: %s\n", ec.message);
    return 1;
  }
  ULARGE_INTEGER ul;
  ul.LowPart = fnow.dwLowDateTime;
  ul.HighPart = fnow.dwHighDateTime;
  for (auto &p : std::filesystem::directory_iterator(pkgtemp)) {
    auto path_ = p.path().wstring();
    if (baulk::IsForceMode || p.is_directory()) {
      bela::fs::RemoveAll(path_, ec);
      continue;
    }
    if (FileIsExpired(path_, ul.QuadPart)) {
      DbgPrint(L"%s expired", path_);
      bela::fs::RemoveAll(path_, ec);
      continue;
    }
  }
  return 0;
}

} // namespace baulk::commands