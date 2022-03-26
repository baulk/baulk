//
#include <bela/fs.hpp>
#include <bela/terminal.hpp>
namespace bela::fs {
inline bool remove_file_hide_attribute(HANDLE FileHandle) {
  FILE_BASIC_INFO bi;
  if (GetFileInformationByHandleEx(FileHandle, FileBasicInfo, &bi, sizeof(bi)) != TRUE) {
    return false;
  }
  bi.FileAttributes = bi.FileAttributes & ~(FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_READONLY);
  return SetFileInformationByHandle(FileHandle, FileBasicInfo, &bi, sizeof(bi)) == TRUE;
}

bool ForceDeleteFile(HANDLE FileHandle, bela::error_code &ec) {
  struct File_disposition_info_ex {
    DWORD Flags;
  };
  DWORD e = NOERROR;
  File_disposition_info_ex Info_ex{0x3};
  constexpr auto FileDispositionInfoExClass = static_cast<FILE_INFO_BY_HANDLE_CLASS>(21);
  if (SetFileInformationByHandle(FileHandle, FileDispositionInfoExClass, &Info_ex, sizeof(Info_ex)) == TRUE) {
    return true;
  }
  if (e = GetLastError(); e == ERROR_ACCESS_DENIED) {
    remove_file_hide_attribute(FileHandle);
    if (SetFileInformationByHandle(FileHandle, FileDispositionInfoExClass, &Info_ex, sizeof(Info_ex)) == TRUE) {
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
    ec = bela::make_error_code_from_system(e, L"SetFileInformationByHandle ");
    return false;
  }
  FILE_DISPOSITION_INFO Info{/* .Delete= */ TRUE};
  if (SetFileInformationByHandle(FileHandle, FileDispositionInfo, &Info, sizeof(Info)) == TRUE) {
    return true;
  }

  if (e = GetLastError(); e == ERROR_ACCESS_DENIED) {
    remove_file_hide_attribute(FileHandle);
    if (SetFileInformationByHandle(FileHandle, FileDispositionInfo, &Info, sizeof(Info)) == TRUE) {
      return true;
    }
    e = GetLastError();
  }
  ec = bela::make_error_code_from_system(e, L"SetFileInformationByHandle(fallback) ");
  return false;
}

bool ForceDeleteFile(std::wstring_view path, bela::error_code &ec) {
  constexpr auto flags = FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT;
  constexpr auto shm = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
  constexpr auto openflags = FILE_READ_ATTRIBUTES | FILE_WRITE_ATTRIBUTES | DELETE;
  // 0x00010000
  auto FileHandle = CreateFileW(path.data(), openflags, shm, nullptr, OPEN_EXISTING, flags, nullptr);
  if (FileHandle == INVALID_HANDLE_VALUE) {
    auto e = GetLastError();
    if (ec.code == ERROR_FILE_NOT_FOUND) {
      return true;
    }
    if (ec.code != ERROR_ACCESS_DENIED) {
      ec = bela::make_error_code_from_system(e);
      return false;
    }
    if (FileHandle = CreateFileW(path.data(), DELETE, shm, nullptr, OPEN_EXISTING, flags, nullptr);
        FileHandle == INVALID_HANDLE_VALUE) {
      ec = bela::make_system_error_code(L"CreateFileW ");
      return false;
    }
  }
  auto closer = bela::finally([&] { CloseHandle(FileHandle); });
  return ForceDeleteFile(FileHandle, ec);
}

bool force_delete_folders(std::wstring_view path, bela::error_code &ec) {
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
      if (!force_delete_folders(child, ec)) {
        return false;
      }
      continue;
    }
    if (!bela::fs::ForceDeleteFile(child, ec)) {
      return false;
    }
  } while (finder.Next());
  return bela::fs::ForceDeleteFile(path, ec);
}

bool ForceDeleteFolders(std::wstring_view path, bela::error_code &ec) {
  if (ForceDeleteFile(path, ec)) {
    return true;
  }
  if (ec.code == ERROR_DIR_NOT_EMPTY) {
    return force_delete_folders(path, ec);
  }
  return false;
}
} // namespace bela::fs
