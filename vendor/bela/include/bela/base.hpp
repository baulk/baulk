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
#include <memory>
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

error_code make_stdc_error_code(errno_t eno, std::wstring_view prefix = L"");
std::wstring resolve_system_error_message(DWORD ec,
                                          std::wstring_view prefix = L"");
inline error_code make_system_error_code(std::wstring_view prefix = L"") {
  error_code ec;
  ec.code = GetLastError();
  ec.message = resolve_system_error_message(ec.code, prefix);
  return ec;
}

std::wstring resolve_module_error_message(const wchar_t *module, DWORD ec,
                                          std::wstring_view prefix);
// bela::fromascii
inline std::wstring fromascii(std::string_view sv) {
  auto sz =
      MultiByteToWideChar(CP_ACP, 0, sv.data(), (int)sv.size(), nullptr, 0);
  std::wstring output;
  output.resize(sz);
  // C++17 must output.data()
  MultiByteToWideChar(CP_ACP, 0, sv.data(), (int)sv.size(), output.data(), sz);
  return output;
}
error_code from_std_error_code(const std::error_code &e,
                               std::wstring_view prefix = L"");
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

// final_act
// https://github.com/microsoft/gsl/blob/ebe7ebfd855a95eb93783164ffb342dbd85cbc27\
// /include/gsl/gsl_util#L85-L89

template <class F> class final_act {
public:
  explicit final_act(F f) noexcept : f_(std::move(f)), invoke_(true) {}

  final_act(final_act &&other) noexcept
      : f_(std::move(other.f_)), invoke_(std::exchange(other.invoke_, false)) {}

  final_act(const final_act &) = delete;
  final_act &operator=(const final_act &) = delete;
  ~final_act() noexcept {
    if (invoke_) {
      f_();
    }
  }

private:
  F f_;
  bool invoke_{true};
};

// finally() - convenience function to generate a final_act
template <class F> inline final_act<F> finally(const F &f) noexcept {
  return final_act<F>(f);
}

template <class F> inline final_act<F> finally(F &&f) noexcept {
  return final_act<F>(std::forward<F>(f));
}

} // namespace bela

#endif
