//
#ifndef BAULK_IO_HPP
#define BAULK_IO_HPP
#include <bela/base.hpp>
#include <bela/path.hpp>

namespace baulk::io {
[[maybe_unused]] constexpr auto MaximumRead = 1024ull * 1024 * 8;
[[maybe_unused]] constexpr auto MaximumLineLength = 1024ull * 64;
bool ReadAll(std::wstring_view file, std::wstring &out, bela::error_code &ec,
             uint64_t maxsize = MaximumRead);
// readline
std::optional<std::wstring> ReadLine(std::wstring_view file,
                                     bela::error_code &ec,
                                     uint64_t maxline = MaximumLineLength);
bool WriteTextU16LE(std::wstring_view text, std::wstring_view file,
                    bela::error_code &ec);
bool WriteText(std::string_view text, std::wstring_view file,
               bela::error_code &ec);
inline bool WriteText(std::wstring_view text, std::wstring_view file,
                      bela::error_code &ec) {
  return WriteText(bela::ToNarrow(text), file, ec);
}
inline bool WriteText(std::u16string_view text, std::wstring_view file,
                      bela::error_code &ec) {
  return WriteText(bela::ToNarrow(text), file, ec);
}
class FilePart {
public:
  FilePart() noexcept = default;
  FilePart(const FilePart &) = delete;
  FilePart &operator=(const FilePart &) = delete;
  FilePart(FilePart &&o) noexcept { transfer_ownership(std::move(o)); }
  FilePart &operator=(FilePart &&o) noexcept {
    transfer_ownership(std::move(o));
    return *this;
  }
  ~FilePart() noexcept { rollback(); }

  bool Finish() {
    if (FileHandle == INVALID_HANDLE_VALUE) {
      SetLastError(ERROR_INVALID_HANDLE);
      return false;
    }
    CloseHandle(FileHandle);
    FileHandle = INVALID_HANDLE_VALUE;
    auto part = bela::StringCat(path, L".part");
    return (MoveFileW(part.data(), path.data()) == TRUE);
  }
  bool Write(const char *data, DWORD len) {
    DWORD dwlen = 0;
    if (WriteFile(FileHandle, data, len, &dwlen, nullptr) != TRUE) {
      return false;
    }
    return len == dwlen;
  }
  static std::optional<FilePart> MakeFilePart(std::wstring_view p,
                                              bela::error_code &ec) {
    FilePart file;
    file.path = bela::PathCat(p); // Path cleanup
    auto part = bela::StringCat(file.path, L".part");
    file.FileHandle = ::CreateFileW(
        part.data(), FILE_GENERIC_READ | FILE_GENERIC_WRITE, FILE_SHARE_READ,
        nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file.FileHandle == INVALID_HANDLE_VALUE) {
      ec = bela::make_system_error_code();
      return std::nullopt;
    }
    return std::make_optional(std::move(file));
  }

private:
  HANDLE FileHandle{INVALID_HANDLE_VALUE};
  std::wstring path;
  void transfer_ownership(FilePart &&other) {
    if (FileHandle != INVALID_HANDLE_VALUE) {
      CloseHandle(FileHandle);
    }
    FileHandle = other.FileHandle;
    other.FileHandle = INVALID_HANDLE_VALUE;
    path = other.path;
    other.path.clear();
  }
  void rollback() noexcept {
    if (FileHandle != INVALID_HANDLE_VALUE) {
      CloseHandle(FileHandle);
      FileHandle = INVALID_HANDLE_VALUE;
      auto part = bela::StringCat(path, L".part");
      DeleteFileW(part.data());
    }
  }
};
} // namespace baulk::io

#endif