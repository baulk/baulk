//
#include <bela/fs.hpp>

namespace bela::fs {
constexpr auto nohideflags = FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_READONLY;
bool Remove(std::wstring_view path, bela::error_code &ec) {
  constexpr auto flags = FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT;
  constexpr auto shm = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
  // 0x00010000
  auto FileHandle = CreateFileW(path.data(), DELETE, shm, nullptr, OPEN_EXISTING, flags, nullptr);
  if (FileHandle == INVALID_HANDLE_VALUE) {
    ec = bela::make_system_error_code(L"CreateFileW ");
    if (ec.code == ERROR_FILE_NOT_FOUND) {
      return true;
    }
    return false;
  }
  auto closer = bela::finally([&] { CloseHandle(FileHandle); });
  struct _File_disposition_info_ex {
    DWORD _Flags;
  };
  DWORD e = NOERROR;
  _File_disposition_info_ex _Info_ex{0x3};
  constexpr auto _FileDispositionInfoExClass = static_cast<FILE_INFO_BY_HANDLE_CLASS>(21);
  if (SetFileInformationByHandle(FileHandle, _FileDispositionInfoExClass, &_Info_ex, sizeof(_Info_ex)) == TRUE) {
    return true;
  }
  if (e = GetLastError(); e == ERROR_ACCESS_DENIED) {
    SetFileAttributesW(path.data(), GetFileAttributesW(path.data()) & ~nohideflags);
    if (SetFileInformationByHandle(FileHandle, _FileDispositionInfoExClass, &_Info_ex, sizeof(_Info_ex)) == TRUE) {
      return true;
    }
    e = GetLastError();
  }
  switch (e) {
  case ERROR_INVALID_PARAMETER:
    [[fallthrough]];
  case ERROR_INVALID_FUNCTION:
    [[fallthrough]];
  case ERROR_NOT_SUPPORTED:
    break;
  default:
    ec = bela::from_system_error_code(e, L"SetFileInformationByHandle ");
    return false;
  }
  FILE_DISPOSITION_INFO _Info{/* .Delete= */ TRUE};
  if (SetFileInformationByHandle(FileHandle, FileDispositionInfo, &_Info, sizeof(_Info)) == TRUE) {
    return true;
  }

  if (e = GetLastError(); e == ERROR_ACCESS_DENIED) {
    SetFileAttributesW(path.data(), GetFileAttributesW(path.data()) & ~nohideflags);
    if (SetFileInformationByHandle(FileHandle, FileDispositionInfo, &_Info, sizeof(_Info)) == TRUE) {
      return true;
    }
    e = GetLastError();
  }
  ec = bela::from_system_error_code(e, L"SetFileInformationByHandle(fallback) ");
  return false;
}

bool remove_all_dir(std::wstring_view path, bela::error_code &ec) {
  Finder finder;
  if (!finder.First(path, L"*", ec)) {
    return false;
  }
  do {
    if (finder.Ignore()) {
      continue;
    }
    auto child = bela::StringCat(path, L"\\", finder.Name());
    if (finder.IsDir() && !finder.IsReparsePoint()) { // only normal dir remove it. symlink not
      if (!remove_all_dir(child, ec)) {
        return false;
      }
      continue;
    }
    if (!bela::fs::Remove(child, ec)) {
      return false;
    }
  } while (finder.Next());
  if (!bela::fs::Remove(path, ec)) {
    return false;
  }
  return true;
}

bool RemoveAll(std::wstring_view path, bela::error_code &ec) {
  if (Remove(path, ec)) {
    return true;
  }
  if (ec.code == ERROR_DIR_NOT_EMPTY) {
    return remove_all_dir(path, ec);
  }
  return false;
}
} // namespace bela::fs
