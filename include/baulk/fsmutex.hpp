//
#ifndef BAULK_FSMUTEX_HPP
#define BAULK_FSMUTEX_HPP
#include <bela/base.hpp>
#include <bela/io.hpp>
#include <bela/str_cat.hpp>
#include <bela/terminal.hpp>
#include <filesystem>

namespace baulk {
namespace mutex_internal {
inline bool process_is_running(DWORD pid) {
  if (HANDLE hProcess = OpenProcess(SYNCHRONIZE, FALSE, pid); hProcess != nullptr) {
    CloseHandle(hProcess);
    return true;
  }
  return false;
}
} // namespace mutex_internal
class FsMutex {
public:
  FsMutex(const FsMutex &) = delete;
  FsMutex &operator=(const FsMutex &) = delete;
  FsMutex(FsMutex &&other) { MoveFrom(std::move(other)); }
  FsMutex &operator=(FsMutex &&other) {
    MoveFrom(std::move(other));
    return *this;
  }
  ~FsMutex() { Free(); }

private:
  FsMutex(HANDLE fileHandle, std::wstring_view filePath_) : FileHandle(fileHandle), filePath(filePath_) {}
  void Free() {
    if (FileHandle == INVALID_HANDLE_VALUE) {
      return;
    }
    CloseHandle(FileHandle);
    FileHandle = INVALID_HANDLE_VALUE;
    DeleteFileW(filePath.data());
    filePath.clear();
  }
  void MoveFrom(FsMutex &&other) {
    Free();
    FileHandle = other.FileHandle;
    filePath = std::move(other.filePath);
    other.FileHandle = INVALID_HANDLE_VALUE;
    other.filePath.clear();
  }
  friend std::optional<FsMutex> MakeFsMutex(std::wstring_view pidfile, bela::error_code &ec);
  std::wstring filePath;
  HANDLE FileHandle{INVALID_HANDLE_VALUE};
};

inline std::optional<FsMutex> MakeFsMutex(std::wstring_view pidfile, bela::error_code &ec) {
  if (auto line = bela::io::ReadLine(pidfile, ec); line) {
    DWORD pid = 0;
    if (bela::SimpleAtoi(*line, &pid) && mutex_internal::process_is_running(pid)) {
      ec = bela::make_error_code(bela::ErrGeneral, L"is running. pid= ", pid);
      return std::nullopt;
    }
  }
  auto FileHandle = ::CreateFileW(pidfile.data(), FILE_GENERIC_READ | FILE_GENERIC_WRITE, FILE_SHARE_READ, nullptr,
                                  CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
  if (FileHandle == INVALID_HANDLE_VALUE) {
    ec = bela::make_system_error_code(L"CreateFsMutex: ");
    return std::nullopt;
  }
  bela::basic_alphanum<char8_t> an(GetCurrentProcessId());
  DWORD written = 0;
  if (WriteFile(FileHandle, an.data(), static_cast<DWORD>(an.size()), &written, nullptr) != TRUE) {
    ec = bela::make_system_error_code();
    CloseHandle(FileHandle);
    FileHandle = INVALID_HANDLE_VALUE;
    DeleteFileW(pidfile.data());
    return std::nullopt;
  }
  // This is done deliberately here to prevent users from calling the FsMutex constructor by mistake.
  FsMutex mtx(FileHandle, pidfile);
  return std::make_optional<FsMutex>(std::move(mtx));
}

} // namespace baulk

#endif