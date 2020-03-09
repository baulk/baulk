//////////
#include <bela/base.hpp>
#include <bela/finaly.hpp>
#include <bela/match.hpp>

namespace bela {
//
bool LookupRealPath(std::wstring_view src, std::wstring &target) {
  // GetFinalPathNameByHandleW
  auto hFile = CreateFileW(src.data(), GENERIC_READ, FILE_SHARE_READ, nullptr,
                           OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
  if (hFile == INVALID_HANDLE_VALUE) {
    return false;
  }
  auto closer = bela::finally([&] {
    if (hFile != INVALID_HANDLE_VALUE) {
      CloseHandle(hFile);
    }
  });
  auto len = GetFinalPathNameByHandleW(hFile, nullptr, 0, VOLUME_NAME_DOS);
  if (len == 0) {
    return false;
  }
  std::wstring buf;
  buf.resize(len);
  len = GetFinalPathNameByHandleW(hFile, buf.data(), len, VOLUME_NAME_DOS);
  if (len == 0) {
    return false;
  }
  buf.resize(len);
  if (bela::StartsWith(buf, L"\\\\?\\UNC\\")) {
    target = buf.substr(7);
    return true;
  }
  if (bela::StartsWith(buf, L"\\\\?\\")) {
    target = buf.substr(4);
    return true;
  }
  target.assign(std::move(buf));
  return true;
}
} // namespace bela