// pkgclean command cleanup pkg cache
#include <bela/terminal.hpp>
#include "baulk.hpp"
#include "commands.hpp"
#include "fs.hpp"
#include "launcher.hpp"

namespace baulk::commands {

bool FileIsExpired(std::wstring_view file, const FILETIME *fnow) {
  FILETIME ftCreate, ftAccess, ftWrite;
  auto FileHandle =
      CreateFileW(file.data(), 0, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr,
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
  constexpr auto expires = 30 * 24 * 3600 * 1000LL;
  auto diffInTicks = reinterpret_cast<const LARGE_INTEGER *>(fnow)->QuadPart -
                     reinterpret_cast<const LARGE_INTEGER *>(&ftCreate)->QuadPart;
  auto diffInMillis = diffInTicks / 10000;
  return diffInMillis > expires;
}

int cmd_cleancache(const argv_t &argv) {
  auto pkgtemp = bela::StringCat(baulk::BaulkRoot(), L"\\", baulk::BaulkPkgTmpDir);
  bela::error_code ec;
  SYSTEMTIME now;
  GetSystemTime(&now);
  FILETIME fnow;
  if (SystemTimeToFileTime(&now, &fnow) != TRUE) {
    auto ec = bela::make_system_error_code();
    bela::FPrintF(stderr, L"SystemTimeToFileTime: %s\n", ec.message);
    return 1;
  }
  for (auto &p : std::filesystem::directory_iterator(pkgtemp)) {
    auto path_ = p.path().wstring();
    if (baulk::IsForceMode) {
      baulk::fs::PathRemoveEx(path_, ec);
      continue;
    }
    if (FileIsExpired(path_, &fnow)) {
      DbgPrint(L"%s expired", path_);
      baulk::fs::PathRemoveEx(path_, ec);
      continue;
    }
  }
  return 0;
}

} // namespace baulk::commands