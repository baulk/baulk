//
#ifndef BELA_SUBSITUTE_HPP
#define BELA_SUBSITUTE_HPP
#pragma once
#include "ascii.hpp"
#include "strcat.hpp" //AlphaNum
#include "algorithm.hpp"

namespace bela {

namespace substitute_internal {
void SubstituteAndAppendArray(std::wstring *output, std::wstring_view format,
                              const std::wstring_view *args_array, size_t num_args);
}
inline void SubstituteAndAppend(std::wstring *output, std::wstring_view format) {
  substitute_internal::SubstituteAndAppendArray(output, format, nullptr, 0);
}

inline void SubstituteAndAppend(std::wstring *output, std::wstring_view format,
                                const AlphaNum &a0) {
  const std::wstring_view args[] = {a0.Piece()};
  substitute_internal::SubstituteAndAppendArray(output, format, args, bela::ArrayLength(args));
}

inline void SubstituteAndAppend(std::wstring *output, std::wstring_view format, const AlphaNum &a0,
                                const AlphaNum &a1) {
  const std::wstring_view args[] = {a0.Piece(), a1.Piece()};
  substitute_internal::SubstituteAndAppendArray(output, format, args, bela::ArrayLength(args));
}

inline void SubstituteAndAppend(std::wstring *output, std::wstring_view format, const AlphaNum &a0,
                                const AlphaNum &a1, const AlphaNum &a2) {
  const std::wstring_view args[] = {a0.Piece(), a1.Piece(), a2.Piece()};
  substitute_internal::SubstituteAndAppendArray(output, format, args, bela::ArrayLength(args));
}

inline void SubstituteAndAppend(std::wstring *output, std::wstring_view format, const AlphaNum &a0,
                                const AlphaNum &a1, const AlphaNum &a2, const AlphaNum &a3) {
  const std::wstring_view args[] = {a0.Piece(), a1.Piece(), a2.Piece(), a3.Piece()};
  substitute_internal::SubstituteAndAppendArray(output, format, args, bela::ArrayLength(args));
}

inline void SubstituteAndAppend(std::wstring *output, std::wstring_view format, const AlphaNum &a0,
                                const AlphaNum &a1, const AlphaNum &a2, const AlphaNum &a3,
                                const AlphaNum &a4) {
  const std::wstring_view args[] = {a0.Piece(), a1.Piece(), a2.Piece(), a3.Piece(), a4.Piece()};
  substitute_internal::SubstituteAndAppendArray(output, format, args, bela::ArrayLength(args));
}

inline void SubstituteAndAppend(std::wstring *output, std::wstring_view format, const AlphaNum &a0,
                                const AlphaNum &a1, const AlphaNum &a2, const AlphaNum &a3,
                                const AlphaNum &a4, const AlphaNum &a5) {
  const std::wstring_view args[] = {a0.Piece(), a1.Piece(), a2.Piece(),
                                    a3.Piece(), a4.Piece(), a5.Piece()};
  substitute_internal::SubstituteAndAppendArray(output, format, args, bela::ArrayLength(args));
}

inline void SubstituteAndAppend(std::wstring *output, std::wstring_view format, const AlphaNum &a0,
                                const AlphaNum &a1, const AlphaNum &a2, const AlphaNum &a3,
                                const AlphaNum &a4, const AlphaNum &a5, const AlphaNum &a6) {
  const std::wstring_view args[] = {a0.Piece(), a1.Piece(), a2.Piece(), a3.Piece(),
                                    a4.Piece(), a5.Piece(), a6.Piece()};
  substitute_internal::SubstituteAndAppendArray(output, format, args, bela::ArrayLength(args));
}

inline void SubstituteAndAppend(std::wstring *output, std::wstring_view format, const AlphaNum &a0,
                                const AlphaNum &a1, const AlphaNum &a2, const AlphaNum &a3,
                                const AlphaNum &a4, const AlphaNum &a5, const AlphaNum &a6,
                                const AlphaNum &a7) {
  const std::wstring_view args[] = {a0.Piece(), a1.Piece(), a2.Piece(), a3.Piece(),
                                    a4.Piece(), a5.Piece(), a6.Piece(), a7.Piece()};
  substitute_internal::SubstituteAndAppendArray(output, format, args, bela::ArrayLength(args));
}

inline void SubstituteAndAppend(std::wstring *output, std::wstring_view format, const AlphaNum &a0,
                                const AlphaNum &a1, const AlphaNum &a2, const AlphaNum &a3,
                                const AlphaNum &a4, const AlphaNum &a5, const AlphaNum &a6,
                                const AlphaNum &a7, const AlphaNum &a8) {
  const std::wstring_view args[] = {a0.Piece(), a1.Piece(), a2.Piece(), a3.Piece(), a4.Piece(),
                                    a5.Piece(), a6.Piece(), a7.Piece(), a8.Piece()};
  substitute_internal::SubstituteAndAppendArray(output, format, args, bela::ArrayLength(args));
}

inline void SubstituteAndAppend(std::wstring *output, std::wstring_view format, const AlphaNum &a0,
                                const AlphaNum &a1, const AlphaNum &a2, const AlphaNum &a3,
                                const AlphaNum &a4, const AlphaNum &a5, const AlphaNum &a6,
                                const AlphaNum &a7, const AlphaNum &a8, const AlphaNum &a9) {
  const std::wstring_view args[] = {a0.Piece(), a1.Piece(), a2.Piece(), a3.Piece(), a4.Piece(),
                                    a5.Piece(), a6.Piece(), a7.Piece(), a8.Piece(), a9.Piece()};
  substitute_internal::SubstituteAndAppendArray(output, format, args, bela::ArrayLength(args));
}

[[nodiscard]] inline std::wstring Substitute(std::wstring_view format) {
  std::wstring result;
  SubstituteAndAppend(&result, format);
  return result;
}

[[nodiscard]] inline std::wstring Substitute(std::wstring_view format, const AlphaNum &a0) {
  std::wstring result;
  SubstituteAndAppend(&result, format, a0);
  return result;
}

[[nodiscard]] inline std::wstring Substitute(std::wstring_view format, const AlphaNum &a0,
                                             const AlphaNum &a1) {
  std::wstring result;
  SubstituteAndAppend(&result, format, a0, a1);
  return result;
}

[[nodiscard]] inline std::wstring Substitute(std::wstring_view format, const AlphaNum &a0,
                                             const AlphaNum &a1, const AlphaNum &a2) {
  std::wstring result;
  SubstituteAndAppend(&result, format, a0, a1, a2);
  return result;
}

[[nodiscard]] inline std::wstring Substitute(std::wstring_view format, const AlphaNum &a0,
                                             const AlphaNum &a1, const AlphaNum &a2,
                                             const AlphaNum &a3) {
  std::wstring result;
  SubstituteAndAppend(&result, format, a0, a1, a2, a3);
  return result;
}

[[nodiscard]] inline std::wstring Substitute(std::wstring_view format, const AlphaNum &a0,
                                             const AlphaNum &a1, const AlphaNum &a2,
                                             const AlphaNum &a3, const AlphaNum &a4) {
  std::wstring result;
  SubstituteAndAppend(&result, format, a0, a1, a2, a3, a4);
  return result;
}

[[nodiscard]] inline std::wstring Substitute(std::wstring_view format, const AlphaNum &a0,
                                             const AlphaNum &a1, const AlphaNum &a2,
                                             const AlphaNum &a3, const AlphaNum &a4,
                                             const AlphaNum &a5) {
  std::wstring result;
  SubstituteAndAppend(&result, format, a0, a1, a2, a3, a4, a5);
  return result;
}

[[nodiscard]] inline std::wstring Substitute(std::wstring_view format, const AlphaNum &a0,
                                             const AlphaNum &a1, const AlphaNum &a2,
                                             const AlphaNum &a3, const AlphaNum &a4,
                                             const AlphaNum &a5, const AlphaNum &a6) {
  std::wstring result;
  SubstituteAndAppend(&result, format, a0, a1, a2, a3, a4, a5, a6);
  return result;
}

[[nodiscard]] inline std::wstring Substitute(std::wstring_view format, const AlphaNum &a0,
                                             const AlphaNum &a1, const AlphaNum &a2,
                                             const AlphaNum &a3, const AlphaNum &a4,
                                             const AlphaNum &a5, const AlphaNum &a6,
                                             const AlphaNum &a7) {
  std::wstring result;
  SubstituteAndAppend(&result, format, a0, a1, a2, a3, a4, a5, a6, a7);
  return result;
}

[[nodiscard]] inline std::wstring Substitute(std::wstring_view format, const AlphaNum &a0,
                                             const AlphaNum &a1, const AlphaNum &a2,
                                             const AlphaNum &a3, const AlphaNum &a4,
                                             const AlphaNum &a5, const AlphaNum &a6,
                                             const AlphaNum &a7, const AlphaNum &a8) {
  std::wstring result;
  SubstituteAndAppend(&result, format, a0, a1, a2, a3, a4, a5, a6, a7, a8);
  return result;
}

[[nodiscard]] inline std::wstring
Substitute(std::wstring_view format, const AlphaNum &a0, const AlphaNum &a1, const AlphaNum &a2,
           const AlphaNum &a3, const AlphaNum &a4, const AlphaNum &a5, const AlphaNum &a6,
           const AlphaNum &a7, const AlphaNum &a8, const AlphaNum &a9) {
  std::wstring result;
  SubstituteAndAppend(&result, format, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9);
  return result;
}
} // namespace bela

#endif
