//////////
// Bela windows basic defined
// Windows error_code impl
#ifndef BELA_BASE_HPP
#define BELA_BASE_HPP
#include "__basal/basal.hpp"
#include <optional>
#include <vector>
#include <system_error>
#include <memory>
#include "types.hpp"
#include "str_cat.hpp"

namespace bela {
// Constructs an bela::error_code object from a value of AlphaNum
[[nodiscard]] inline error_code make_error_code(const AlphaNum &a) { return error_code{a.Piece(), ErrGeneral}; }
// Constructs an bela::error_code object from a value of AlphaNum
[[nodiscard]] inline error_code make_error_code(long code, const AlphaNum &a) { return error_code{a.Piece(), code}; }
// Constructs an bela::error_code object from a value of AlphaNum
[[nodiscard]] inline error_code make_error_code(long code, const AlphaNum &a, const AlphaNum &b) {
  return error_code{StringCat(a, b), code};
}
// Constructs an bela::error_code object from a value of AlphaNum
[[nodiscard]] inline error_code make_error_code(long code, const AlphaNum &a, const AlphaNum &b, const AlphaNum &c) {
  return error_code{StringCat(a, b, c), code};
}
// Constructs an bela::error_code object from a value of AlphaNum
[[nodiscard]] inline error_code make_error_code(long code, const AlphaNum &a, const AlphaNum &b, const AlphaNum &c,
                                                const AlphaNum &d) {
  return error_code{StringCat(a, b, c, d), code};
}
// Constructs an bela::error_code object from a value of AlphaNum
template <typename... AV>
[[nodiscard]] error_code make_error_code(long code, const AlphaNum &a, const AlphaNum &b, const AlphaNum &c,
                                         const AlphaNum &d, const AV &...av) {
  return error_code{strings_internal::CatPieces(
                        {a.Piece(), b.Piece(), c.Piece(), d.Piece(), static_cast<const AlphaNum &>(av).Piece()...}),
                    code};
}

// https://github.com/microsoft/wil/blob/master/include/wil/stl.h#L38
template <typename T> struct secure_allocator : public std::allocator<T> {
  template <typename Other> struct rebind { typedef secure_allocator<Other> other; };

  secure_allocator() : std::allocator<T>() {}

  ~secure_allocator() = default;

  secure_allocator(const secure_allocator &a) : std::allocator<T>(a) {}

  template <class U> secure_allocator(const secure_allocator<U> &a) : std::allocator<T>(a) {}

  T *allocate(size_t n) { return std::allocator<T>::allocate(n); }

  void deallocate(T *p, size_t n) {
    SecureZeroMemory(p, sizeof(T) * n);
    std::allocator<T>::deallocate(p, n);
  }
};

//! `bela::secure_vector` will be securely zeroed before deallocation.
template <typename Type> using secure_vector = std::vector<Type, bela::secure_allocator<Type>>;
//! `bela::secure_wstring` will be securely zeroed before deallocation.
using secure_wstring = std::basic_string<wchar_t, std::char_traits<wchar_t>, bela::secure_allocator<wchar_t>>;
//! `bela::secure_string` will be securely zeroed before deallocation.
using secure_string = std::basic_string<char, std::char_traits<char>, bela::secure_allocator<char>>;

// final_act
// https://github.com/microsoft/gsl/blob/ebe7ebfd855a95eb93783164ffb342dbd85cbc27\
// /include/gsl/gsl_util#L85-L89

template <class F> class final_act {
public:
  explicit final_act(F f) noexcept : f_(std::move(f)), invoke_(true) {}

  final_act(final_act &&other) noexcept : f_(std::move(other.f_)), invoke_(std::exchange(other.invoke_, false)) {}

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
template <class F> inline final_act<F> finally(const F &f) noexcept { return final_act<F>(f); }

template <class F> inline final_act<F> finally(F &&f) noexcept { return final_act<F>(std::forward<F>(f)); }

} // namespace bela

#endif
