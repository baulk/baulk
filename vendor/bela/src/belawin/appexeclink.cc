//////////
///
#include <bela/path.hpp>
#include <bela/finaly.hpp>
#include <winioctl.h>
#include "reparsepoint_internal.hpp"

namespace bela {
bool LookupAppExecLinkTarget(std::wstring_view src, AppExecTarget &ae) {
  HANDLE hFile = INVALID_HANDLE_VALUE;
  REPARSE_DATA_BUFFER *buf = nullptr;
  auto closer = bela::finally([&] {
    if (hFile != nullptr) {
      CloseHandle(hFile);
    }
    if (buf != nullptr) {
      HeapFree(GetProcessHeap(), 0, buf);
    }
  });
  buf = reinterpret_cast<REPARSE_DATA_BUFFER *>(
      HeapAlloc(GetProcessHeap(), 0, MAXIMUM_REPARSE_DATA_BUFFER_SIZE));
  if (buf == nullptr) {
    return false;
  }
  hFile = CreateFileW(
      src.data(), 0, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
      nullptr, OPEN_EXISTING,
      FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT, nullptr);
  if (hFile == INVALID_HANDLE_VALUE) {
    return false;
  }
  DWORD dwBytes = 0;
  if (DeviceIoControl(hFile, FSCTL_GET_REPARSE_POINT, nullptr, 0, buf,
                      MAXIMUM_REPARSE_DATA_BUFFER_SIZE, &dwBytes,
                      nullptr) != TRUE) {
    return false;
  }
  if (buf->ReparseTag != IO_REPARSE_TAG_APPEXECLINK) {
    return false;
  }
  LPWSTR szString = (LPWSTR)buf->AppExecLinkReparseBuffer.StringList;
  std::vector<LPWSTR> strv;
  for (ULONG i = 0; i < buf->AppExecLinkReparseBuffer.StringCount; i++) {
    strv.push_back(szString);
    szString += wcslen(szString) + 1;
  }
  if (!strv.empty()) {
    ae.pkid = strv[0];
  }
  if (strv.size() > 1) {
    ae.appuserid = strv[1];
  }
  if (strv.size() > 2) {
    ae.target = strv[2];
  }
  return true;
}
} // namespace bela
