///
#ifndef BELA_STRIP_HPP
#define BELA_STRIP_HPP
#pragma once
#include "match.hpp"

namespace bela {
inline bool ConsumePrefix(std::wstring_view *str, std::wstring_view expected) {
  if (!bela::StartsWith(*str, expected)) {
    return false;
  }
  str->remove_prefix(expected.size());
  return true;
}
inline bool ConsumeSuffix(std::wstring_view *str, std::wstring_view expected) {
  if (!bela::EndsWith(*str, expected)) {
    return false;
  }
  str->remove_suffix(expected.size());
  return true;
}

[[nodiscard]] inline std::wstring_view StripPrefix(std::wstring_view str,
                                                   std::wstring_view prefix) {
  if (bela::StartsWith(str, prefix)) {
    str.remove_prefix(prefix.size());
  }
  return str;
}

inline std::wstring_view StripSuffix(std::wstring_view str, std::wstring_view suffix) {
  if (bela::EndsWith(str, suffix)) {
    str.remove_suffix(suffix.size());
  }
  return str;
}

} // namespace bela

#endif
