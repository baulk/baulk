//////////
#include <bela/base.hpp>
#include <bela/match.hpp>

namespace bela {
std::optional<std::wstring> RealPath(std::wstring_view src,
                                     bela::error_code &ec) {
  auto hFile = CreateFileW(
      src.data(), 0, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
      nullptr, OPEN_EXISTING,
      FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT, nullptr);
  // FILE_FLAG_BACKUP_SEMANTICS open directory require
  if (hFile == INVALID_HANDLE_VALUE) {
    ec = bela::make_system_error_code();
    return std::nullopt;
  }
  auto closer = bela::finally([&] {
    if (hFile != INVALID_HANDLE_VALUE) {
      CloseHandle(hFile);
    }
  });
  auto len = GetFinalPathNameByHandleW(hFile, nullptr, 0, VOLUME_NAME_DOS);
  if (len == 0) {
    ec = bela::make_system_error_code();
    return std::nullopt;
  }
  std::wstring buf;
  buf.resize(len);
  if (len = GetFinalPathNameByHandleW(hFile, buf.data(), len, VOLUME_NAME_DOS);
      len == 0) {
    ec = bela::make_system_error_code();
    return std::nullopt;
  }
  buf.resize(len);
  if (bela::StartsWith(buf, L"\\\\?\\UNC\\")) {
    return std::make_optional(buf.substr(7));
  }
  if (bela::StartsWith(buf, L"\\\\?\\")) {
    return std::make_optional(buf.substr(4));
  }
  return std::make_optional(std::move(buf));
}

std::optional<std::wstring> Executable(bela::error_code &ec) {
  std::wstring buffer;
  buffer.resize(MAX_PATH);
  auto X = GetModuleFileNameW(nullptr, buffer.data(), MAX_PATH);
  if (X == 0) {
    ec = bela::make_system_error_code();
    return std::nullopt;
  }
  if (X <= MAX_PATH) {
    buffer.resize(X);
    return std::make_optional(std::move(buffer));
  }
  buffer.resize(X);
  auto Y = GetModuleFileNameW(nullptr, buffer.data(), X);
  if (Y == 0) {
    ec = bela::make_system_error_code();
    return std::nullopt;
  }
  if (Y <= X) {
    buffer.resize(Y);
    return std::make_optional(std::move(buffer));
  }
  ec = bela::make_error_code(-1, L"Executable unable allocate more buffer");
  return std::nullopt;
}

std::optional<std::wstring> ExecutableFinalPath(bela::error_code &ec) {
  if (auto exe = bela::Executable(ec); exe) {
    return RealPath(*exe, ec);
  }
  return std::nullopt;
}

} // namespace bela
