#include <bela/match.hpp>
#include <bela/path.hpp>
#include <baulk/archive.hpp>
#include <filesystem>

namespace baulk::archive {
inline void close_file(HANDLE &hFile) {
  if (hFile != INVALID_HANDLE_VALUE) {
    CloseHandle(hFile);
    hFile = INVALID_HANDLE_VALUE;
  }
}
// https://docs.microsoft.com/en-us/windows/desktop/api/fileapi/nf-fileapi-setfiletime
inline bool chtimes(HANDLE fd, bela::Time t, bela::error_code &ec) {
  auto ft = bela::ToFileTime(t);
  if (::SetFileTime(fd, &ft, &ft, &ft) != TRUE) {
    ec = bela::make_system_error_code(L"SetFileTime ");
    return false;
  }
  return true;
}

inline bool discard_fd(HANDLE &fd) {
  if (fd == INVALID_HANDLE_VALUE) {
    return false;
  }
  if (fd == INVALID_HANDLE_VALUE) {
    return false;
  }
  // From newer Windows SDK than currently used to build vctools:
  // #define FILE_DISPOSITION_FLAG_DELETE                     0x00000001
  // #define FILE_DISPOSITION_FLAG_POSIX_SEMANTICS            0x00000002

  // typedef struct _FILE_DISPOSITION_INFO_EX {
  //     DWORD Flags;
  // } FILE_DISPOSITION_INFO_EX, *PFILE_DISPOSITION_INFO_EX;

  struct _File_disposition_info_ex {
    DWORD _Flags;
  };
  _File_disposition_info_ex _Info_ex{0x3};

  // FileDispositionInfoEx isn't documented in MSDN at the time of this writing, but is present
  // in minwinbase.h as of at least 10.0.16299.0
  constexpr auto _FileDispositionInfoExClass = static_cast<FILE_INFO_BY_HANDLE_CLASS>(21);
  if (SetFileInformationByHandle(fd, _FileDispositionInfoExClass, &_Info_ex, sizeof(_Info_ex)) == TRUE) {
    close_file(fd);
    return true;
  }
  FILE_DISPOSITION_INFO _Info{/* .Delete= */ TRUE};
  if (SetFileInformationByHandle(fd, FileDispositionInfo, &_Info, sizeof(_Info)) == TRUE) {
    close_file(fd);
    return true;
  }
  return false;
}

File::~File() { close_file(fd); }
File::File(File &&o) noexcept {
  close_file(fd);
  fd = o.fd;
  o.fd = INVALID_HANDLE_VALUE;
}
File &File::operator=(File &&o) noexcept {
  close_file(fd);
  fd = o.fd;
  o.fd = INVALID_HANDLE_VALUE;
  return *this;
}
bool File::Chtimes(bela::Time t, bela::error_code &ec) { return chtimes(fd, t, ec); }
bool File::Discard() { return discard_fd(fd); }

/// WriteFull
bool File::WriteFull(const void *data, size_t bytes, bela::error_code &ec) {
  auto len = static_cast<DWORD>(bytes);
  auto u8d = reinterpret_cast<const uint8_t *>(data);
  DWORD writtenBytes = 0;
  do {
    DWORD dwSize = 0;
    if (WriteFile(fd, u8d + writtenBytes, len - writtenBytes, &dwSize, nullptr) != TRUE) {
      ec = bela::make_system_error_code(L"WriteFile() ");
      return false;
    }
    writtenBytes += dwSize;
  } while (writtenBytes < len);
  return true;
}

std::optional<File> File::NewFile(const fs::path &path, bela::Time modified, bool overwrite_mode,
                                  bela::error_code &ec) {
  std::error_code e;
  if (fs::exists(path, e)) {
    if (!overwrite_mode) {
      ec = bela::make_error_code(ErrGeneral, L"file '", path.native(), L"' exists");
      return std::nullopt;
    }
  } else {
    if (fs::create_directories(path.parent_path(), e); e) {
      ec = bela::from_std_error_code(e, L"create_directories() ");
      return std::nullopt;
    }
  }
  auto fd = CreateFileW(path.c_str(), FILE_GENERIC_READ | FILE_GENERIC_WRITE | GENERIC_READ | GENERIC_WRITE | DELETE,
                        FILE_SHARE_READ, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
  if (fd == INVALID_HANDLE_VALUE) {
    ec = bela::make_system_error_code(L"CreateFileW ");
    return std::nullopt;
  }
  if (!chtimes(fd, modified, ec)) {
    discard_fd(fd);
    return std::nullopt;
  }
  return std::make_optional<File>(fd);
}

bool NewSymlink(const fs::path &path, const fs::path &source, bool overwrite_mode, bela::error_code &ec) {
  std::error_code e;
  if (fs::exists(path, e)) {
    if (!overwrite_mode) {
      ec = bela::make_error_code(ErrGeneral, L"file '", path.native(), L"' exists");
      return false;
    }
    fs::remove_all(path, e);
  } else {
    if (fs::create_directories(path.parent_path(), e); e) {
      ec = bela::from_std_error_code(e, L"create_directories() ");
      return false;
    }
  }
  if (fs::create_symlink(source, path, e); e) {
    ec = bela::from_std_error_code(e, L"create_symlink() ");
    return false;
  }
  return true;
}

bool Chtimes(const fs::path &file, bela::Time t, bela::error_code &ec) {
  auto fd =
      CreateFileW(file.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                  nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr);
  if (fd == INVALID_HANDLE_VALUE) {
    ec = bela::make_system_error_code(L"CreateFileW() ");
    return false;
  }
  auto closer = bela::finally([&] { CloseHandle(fd); });
  return chtimes(fd, t, ec);
}

constexpr bool is_dot_or_separator(wchar_t ch) { return bela::IsPathSeparator(ch) || ch == L'.'; }

std::wstring_view PathRemoveExtension(std::wstring_view p) {
  if (p.empty()) {
    return L".";
  }
  auto i = p.size() - 1;
  for (; i != 0 && is_dot_or_separator(p[i]); i--) {
    p.remove_suffix(1);
  }
  constexpr std::wstring_view extensions[] = {L".tar.gz",   L".tar.bz2", L".tar.xz", L".tar.zst",
                                              L".tar.zstd", L".tar.br",  L".tar.lz4"};
  for (const auto e : extensions) {
    if (bela::EndsWithIgnoreCase(p, e)) {
      return p.substr(0, p.size() - e.size());
    }
  }
  if (auto pos = p.find_last_of(L"\\/."); pos != std::wstring_view::npos && p[pos] == L'.') {
    // if (pos >= 1 && bela::IsPathSeparator(p[pos - 1])) {
    //   return p;
    // }
    return p.substr(0, pos);
  }
  return p;
}

} // namespace baulk::archive