//////////
// Bela windows basic defined
// Windows error_code impl
#ifndef BELA_BASE_HPP
#define BELA_BASE_HPP
#pragma once
#include <SDKDDKVer.h>
#ifndef _WINDOWS_
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN //
#endif
#include <windows.h>
#endif
#include <optional>
#include <string>
#include <string_view>
#include <vector>
#include <system_error>
#include "strcat.hpp"

namespace bela {
enum bela_extend_error_category : long {
  None = 0, // None error
  SkipParse = 0x4001,
  ParseBroken = 0x4002,
  FileSizeTooSmall = 0x4003,
};

struct error_code {
  std::wstring message;
  long code{None};
  const wchar_t *data() const { return message.data(); }
  explicit operator bool() const noexcept { return code != None; }
};

inline bela::error_code make_error_code(long code, const AlphaNum &a) {
  return bela::error_code{std::wstring(a.Piece()), code};
}
inline bela::error_code make_error_code(long code, const AlphaNum &a,
                                        const AlphaNum &b) {
  bela::error_code ec;
  ec.code = code;
  ec.message.reserve(a.Piece().size() + b.Piece().size());
  ec.message.assign(a.Piece()).append(b.Piece());
  return ec;
}
inline bela::error_code make_error_code(long code, const AlphaNum &a,
                                        const AlphaNum &b, const AlphaNum &c) {
  bela::error_code ec;
  ec.code = code;
  ec.message.reserve(a.Piece().size() + b.Piece().size() + c.Piece().size());
  ec.message.assign(a.Piece()).append(b.Piece()).append(c.Piece());
  return ec;
}
inline bela::error_code make_error_code(long code, const AlphaNum &a,
                                        const AlphaNum &b, const AlphaNum &c,
                                        const AlphaNum &d) {
  bela::error_code ec;
  ec.code = code;
  ec.message.reserve(a.Piece().size() + b.Piece().size() + c.Piece().size() +
                     d.Piece().size());
  ec.message.assign(a.Piece()).append(b.Piece()).append(c.Piece()).append(
      d.Piece());
  return ec;
}
template <typename... AV>
bela::error_code make_error_code(long code, const AlphaNum &a,
                                 const AlphaNum &b, const AlphaNum &c,
                                 const AlphaNum &d, AV... av) {
  bela::error_code ec;
  ec.code = code;
  ec.message = strings_internal::CatPieces({a, b, c, d, av...});
  return ec;
}

std::wstring resolve_system_error_code(DWORD ec);
error_code make_stdc_error_code(errno_t eno);

inline error_code make_system_error_code() {
  error_code ec;
  ec.code = GetLastError();
  ec.message = resolve_system_error_code(ec.code);
  return ec;
}

inline error_code from_std_error_code(const std::error_code &e) {
  error_code ec;
  ec.code = e.value();
  ec.message = bela::ToWide(e.message());
  return ec;
}

// https://github.com/microsoft/wil/blob/master/include/wil/stl.h#L38
template <typename T> struct secure_allocator : public std::allocator<T> {
  template <typename Other> struct rebind {
    typedef secure_allocator<Other> other;
  };

  secure_allocator() : std::allocator<T>() {}

  ~secure_allocator() = default;

  secure_allocator(const secure_allocator &a) : std::allocator<T>(a) {}

  template <class U>
  secure_allocator(const secure_allocator<U> &a) : std::allocator<T>(a) {}

  T *allocate(size_t n) { return std::allocator<T>::allocate(n); }

  void deallocate(T *p, size_t n) {
    SecureZeroMemory(p, sizeof(T) * n);
    std::allocator<T>::deallocate(p, n);
  }
};

//! `bela::secure_vector` will be securely zeroed before deallocation.
template <typename Type>
using secure_vector = std::vector<Type, bela::secure_allocator<Type>>;
//! `bela::secure_wstring` will be securely zeroed before deallocation.
using secure_wstring = std::basic_string<wchar_t, std::char_traits<wchar_t>,
                                         bela::secure_allocator<wchar_t>>;
//! `bela::secure_string` will be securely zeroed before deallocation.
using secure_string = std::basic_string<char, std::char_traits<char>,
                                        bela::secure_allocator<char>>;

} // namespace bela

#endif
