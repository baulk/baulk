//////////
#include <bela/base.hpp>
#include <bela/match.hpp>
#include <string_view>

namespace bela {
// Thanks MSVC STL filesystem
template <class _Ty>
[[nodiscard]] _Ty _Unaligned_load(const void *_Ptr) { // load a _Ty from _Ptr
  static_assert(std::is_trivial_v<_Ty>,
                "Unaligned loads require trivial types");
  _Ty _Tmp;
  std::memcpy(&_Tmp, _Ptr, sizeof(_Tmp));
  return _Tmp;
}

[[nodiscard]] inline bool _Is_drive_prefix(const wchar_t *const _First) {
  // test if _First points to a prefix of the form X:
  // pre: _First points to at least 2 wchar_t instances
  // pre: Little endian
  auto _Value = _Unaligned_load<unsigned int>(_First);
  _Value &=
      0xFFFF'FFDFu; // transform lowercase drive letters into uppercase ones
  _Value -=
      (static_cast<unsigned int>(L':') << (sizeof(wchar_t) * CHAR_BIT)) | L'A';
  return _Value < 26;
}

inline bool
_Is_drive_prefix_with_slash_slash_question(const std::wstring_view text) {
  return text.size() > 6 && bela::StartsWith(text, LR"(\\?\)") &&
         _Is_drive_prefix(text.data() + 4);
}

std::optional<std::wstring> RealPathByHandle(HANDLE FileHandle,
                                             bela::error_code &ec) {
  std::wstring buffer;
  buffer.resize(260); // opt
  DWORD kind = VOLUME_NAME_DOS;
  for (;;) {
    const auto blen = buffer.size();
    auto len = GetFinalPathNameByHandleW(FileHandle, buffer.data(),
                                         static_cast<DWORD>(blen), kind);
    if (len == 0) {
      auto e = GetLastError();
      if (e == ERROR_PATH_NOT_FOUND && kind == VOLUME_NAME_DOS) {
        kind = VOLUME_NAME_NT;
        continue;
      }
      ec.code = e;
      ec.message = bela::resolve_system_error_message(ec.code);
      return std::nullopt;
    }
    buffer.resize(len);
    if (len < static_cast<DWORD>(blen)) {
      break;
    }
  }
  if (kind != VOLUME_NAME_DOS) {
    // result is in the NT namespace, so apply the DOS to NT namespace prefix
    constexpr std::wstring_view ntprefix = LR"(\\?\GLOBALROOT)";
    return bela::StringCat(ntprefix, buffer);
  }
  // '\\?\C:\Path'
  if (_Is_drive_prefix_with_slash_slash_question(buffer)) {
    return std::make_optional(buffer.substr(4));
  }
  if (bela::StartsWithIgnoreCase(buffer, LR"(\\?\UNC\)")) {
    std::wstring_view sv(buffer);
    return std::make_optional(bela::StringCat(LR"(\\)", sv.substr(6)));
  }
  return std::make_optional(std::move(buffer));
}

std::optional<std::wstring> RealPath(std::wstring_view src,
                                     bela::error_code &ec) {
  auto FileHandle = CreateFileW(
      src.data(), 0, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
      nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr);
  // FILE_FLAG_BACKUP_SEMANTICS open directory require
  if (FileHandle == INVALID_HANDLE_VALUE) {
    ec = bela::make_system_error_code();
    return std::nullopt;
  }
  auto closer = bela::finally([&] { CloseHandle(FileHandle); });
  return bela::RealPathByHandle(FileHandle, ec);
}

std::optional<std::wstring> Executable(bela::error_code &ec) {
  std::wstring buffer;
  buffer.resize(MAX_PATH);
  for (;;) {
    const auto blen = buffer.size();
    auto n =
        GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(blen));
    if (n == 0) {
      ec = bela::make_system_error_code();
      return std::nullopt;
    }
    buffer.resize(n);
    if (n < static_cast<DWORD>(blen)) {
      break;
    }
  }
  return std::make_optional(std::move(buffer));
}

std::optional<std::wstring> ExecutableFinalPath(bela::error_code &ec) {
  if (auto exe = bela::Executable(ec); exe) {
    return RealPath(*exe, ec);
  }
  return std::nullopt;
}

} // namespace bela
