#include <bela/time.hpp>
#include <bela/terminal.hpp>

bool SetFolderTime(std::wstring_view dir, bela::Time t, bela::error_code &ec) {
  auto FileHandle =
      CreateFileW(dir.data(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                  nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr);
  if (FileHandle == INVALID_HANDLE_VALUE) {
    ec = bela::make_system_error_code(L"CreateFileW() ");
    return false;
  }
  auto closer = bela::finally([&] { CloseHandle(FileHandle); });
  auto ft = bela::ToFileTime(t);
  if (::SetFileTime(FileHandle, &ft, &ft, &ft) != TRUE) {
    ec = bela::make_system_error_code(L"SetFileTime ");
    return false;
  }
  return true;
}

int wmain(int argc, wchar_t **argv) {
  if (argc < 2) {
    bela::FPrintF(stderr, L"usage: %s dir\n", argv[0]);
    return 1;
  }
  bela::error_code ec;
  auto now = bela::Now();
  if (!SetFolderTime(argv[1], now, ec)) {
    bela::FPrintF(stderr, L"change dir time: %s\n", ec);
    return 1;
  }
  bela::FPrintF(stderr, L"change dir time success\n");
  return 0;
}