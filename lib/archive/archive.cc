///
#include <memory_resource>
#include <filesystem>
#include <archive.hpp>
#include <bela/datetime.hpp>
#include <bela/path.hpp>

namespace baulk::archive {
// https://en.cppreference.com/w/cpp/memory/unsynchronized_pool_resource
// https://quuxplusone.github.io/blog/2018/06/05/libcpp-memory-resource/
// https://www.bfilipek.com/2020/06/pmr-hacking.html

#ifdef PARALLEL_UNZIP
std::pmr::synchronized_pool_resource pool_;
#else
std::pmr::unsynchronized_pool_resource pool_;
#endif

void Buffer::Free() {
  if (data_ != nullptr) {
    pool_.deallocate(data_, capacity_);
    data_ = nullptr;
    capacity_ = 0;
  }
}

void Buffer::grow(size_t n) {
  if (n <= capacity_) {
    return;
  }
  auto b = reinterpret_cast<uint8_t *>(pool_.allocate(n));
  if (size_ != 0) {
    memcpy(b, data_, n);
  }
  if (data_ != nullptr) {
    pool_.deallocate(data_, capacity_);
  }
  data_ = b;
  capacity_ = n;
}

FILETIME TimeToFileTime(bela::Time t) {
  FILETIME ft{};
  auto parts = bela::Split(t);
  auto tick = (parts.sec - 11644473600ll) * 10000000 + parts.nsec / 100;
  ft.dwHighDateTime = static_cast<DWORD>(tick << 32);
  ft.dwLowDateTime = static_cast<DWORD>(tick);
  return ft;
}
// https://docs.microsoft.com/en-us/windows/desktop/api/fileapi/nf-fileapi-setfiletime
bool FD::SetFileTime(bela::Time t, bela::error_code &ec) {
  auto filetime = TimeToFileTime(t);
  if (::SetFileTime(fd, &filetime, nullptr, nullptr) != TRUE) {
    ec = bela::make_system_error_code(L"SetFileTime ");
    return false;
  }
  return true;
}
bool FD::Discard() {
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
  if (SetFileInformationByHandle(fd, _FileDispositionInfoExClass, &_Info_ex, sizeof(_Info_ex))) {
    Free();
    return true;
  }
  FILE_DISPOSITION_INFO _Info{/* .Delete= */ TRUE};
  if (SetFileInformationByHandle(fd, FileDispositionInfo, &_Info, sizeof(_Info))) {
    Free();
    return true;
  }
  return false;
}

bool FD::Write(const void *data, size_t bytes, bela::error_code &ec) {
  auto p = reinterpret_cast<const char *>(data);
  while (bytes != 0) {
    DWORD dwSize = 0;
    if (WriteFile(fd, p, static_cast<DWORD>(bytes), &dwSize, nullptr) != TRUE) {
      ec = bela::make_system_error_code(L"WriteFull ");
      return false;
    }
    bytes -= dwSize;
    p += dwSize;
  }
  return true;
}

std::optional<FD> NewFD(std::wstring_view path, bela::error_code &ec, bool overwrite) {
  std::filesystem::path p(path);
  std::error_code sec;
  if (std::filesystem::exists(p, sec)) {
    if (!overwrite) {
      ec = bela::make_error_code(1, L"file '", p.filename().wstring(), L"' exists");
      return std::nullopt;
    }
  } else {
    std::filesystem::create_directories(p.parent_path(), sec);
  }
  auto fd = CreateFileW(path.data(), FILE_GENERIC_READ | FILE_GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS,
                        FILE_ATTRIBUTE_NORMAL, nullptr);
  if (fd == INVALID_HANDLE_VALUE) {
    ec = bela::make_system_error_code(L"CreateFileW ");
    return std::nullopt;
  }
  return std::make_optional<FD>(fd);
}

#ifndef SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE
#define SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE (0x2)
#endif

#define SYMBOLIC_LINK_DIR (SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE | SYMBOLIC_LINK_FLAG_DIRECTORY)

bool NewSymlink(std::wstring_view path, std::wstring_view linkname, bela::error_code &ec, bool overwrite) {
  std::filesystem::path p(path);
  std::error_code sec;
  if (std::filesystem::exists(p, sec)) {
    if (!overwrite) {
      ec = bela::make_error_code(1, L"file '", p.filename().wstring(), L"' exists");
      return false;
    }
    std::filesystem::remove_all(p, sec);
  } else {
    std::filesystem::create_directories(p.parent_path(), sec);
  }
  DWORD flags = linkname.ends_with('/') ? SYMBOLIC_LINK_DIR : SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE;
  if (CreateSymbolicLinkW(path.data(), linkname.data(), flags) != TRUE) {
    ec = bela::make_system_error_code();
    return false;
  }
  return true;
}

} // namespace baulk::archive