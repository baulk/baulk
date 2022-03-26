///
#ifndef BELA_INTERNAL_BASAL_HPP
#define BELA_INTERNAL_BASAL_HPP
#pragma once
#include <SDKDDKVer.h>
#ifndef _WINDOWS_
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN //
#endif
#include <windows.h>
#endif
#include <string>
#include <string_view>
#include <system_error>
#include <compare>

namespace bela {
constexpr long ErrNone = 0;
constexpr long ErrEOF = ERROR_HANDLE_EOF;
constexpr long ErrGeneral = 0x4001;
constexpr long ErrSkipParse = 0x4002;
constexpr long ErrParseBroken = 0x4003;
constexpr long ErrFileTooSmall = 0x4004;
constexpr long ErrFileAlreadyOpened = 0x4005;
constexpr long ErrEnded = 654320;
constexpr long ErrCanceled = 654321;
constexpr long ErrUnimplemented = 654322; // feature not implemented

// bela::error_code is a platform-dependent error code
struct error_code;

// Constructs an bela::error_code object from current context
error_code make_system_error_code(std::wstring_view prefix = L"");
// Constructs an bela::error_code object from errno
error_code make_error_code_from_errno(errno_t eno, std::wstring_view prefix = L"");
// Constructs an bela::error_code object from system error code
error_code make_error_code_from_system(DWORD e, std::wstring_view prefix = L"");
// Constructs an bela::error_code object from std::error_code
error_code make_error_code_from_std(const std::error_code &e, std::wstring_view prefix = L"");
// Constructs an bela::error_code object from
std::wstring resolve_system_error_message(DWORD ec, std::wstring_view prefix = L"");
std::wstring resolve_module_error_message(const wchar_t *moduleName, DWORD ec, std::wstring_view prefix);

// bela::fromascii
[[nodiscard]] inline std::wstring fromascii(std::string_view sv) {
  auto sz = MultiByteToWideChar(CP_ACP, 0, sv.data(), (int)sv.size(), nullptr, 0);
  std::wstring output;
  output.resize(sz);
  // C++17 must output.data()
  MultiByteToWideChar(CP_ACP, 0, sv.data(), (int)sv.size(), output.data(), sz);
  return output;
}

// bela::error_code is a platform-dependent error code
struct error_code {
  std::wstring message;
  long code{ErrNone};
  // -----------
  error_code() noexcept = default;
  error_code(std::wstring_view message_, long code_) noexcept {
    message = message_;
    code = code_;
  }
  error_code(const error_code &) = default;
  error_code &operator=(const error_code &) = default;
  error_code(error_code &&) = default;
  error_code &operator=(error_code &&) = default;
  error_code &operator=(DWORD e) noexcept {
    *this = make_error_code_from_system(e);
    return *this;
  }
  error_code &operator=(errno_t e) noexcept {
    *this = make_error_code_from_errno(e);
    return *this;
  }
  error_code &operator=(const std::error_code &e) {
    *this = make_error_code_from_std(e);
    return *this;
  }
  error_code &assgin(error_code &&o) {
    *this = std::move(o);
    return *this;
  }
  void clear() {
    code = ErrNone;
    message.clear();
  }
  [[nodiscard]] const std::wstring &native() const { return message; }
  [[nodiscard]] const wchar_t *data() const { return message.data(); }
  [[nodiscard]] explicit operator bool() const noexcept { return code != ErrNone; }
  [[nodiscard]] friend bool operator==(const error_code &_Left, const error_code &_Right) noexcept {
    return _Left.code == _Right.code;
  }
  [[nodiscard]] friend bool operator==(const error_code &_Left, const long _Code) noexcept {
    return _Left.code == _Code;
  }
  [[nodiscard]] friend std::strong_ordering operator<=>(const error_code &_Left, long _Code) noexcept {
    return _Left.code <=> _Code;
  }
  [[nodiscard]] friend std::strong_ordering operator<=>(const error_code &_Left, const error_code &_Right) noexcept {
    return _Left.code <=> _Right.code;
  }
};

// make_error_code_from_system convert from Windows error code convert to error_code
[[nodiscard]] inline error_code make_error_code_from_system(DWORD e, std::wstring_view prefix) {
  return error_code{resolve_system_error_message(e, prefix), static_cast<long>(e)};
}
// make_system_error_code call GetLastError get error code convert to error_code
[[nodiscard]] inline error_code make_system_error_code(std::wstring_view prefix) {
  return make_error_code_from_system(GetLastError(), prefix);
}

} // namespace bela

#endif