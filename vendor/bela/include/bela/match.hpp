////////////
#ifndef BELA_MATCH_HPP
#define BELA_MATCH_HPP
#pragma once
#include <cstring>
#include <string_view>

namespace bela {

inline bool memequal(const wchar_t *l, const wchar_t *r, size_t n) {
  return memcmp(l, r, sizeof(wchar_t) * n);
}

inline bool StartsWith(std::wstring_view text, std::wstring_view prefix) {
  return prefix.empty() ||
         (text.size() >= prefix.size() &&
          memequal(text.data(), prefix.data(), prefix.size()) == 0);
}
inline bool EndsWith(std::wstring_view text, std::wstring_view suffix) {
  return suffix.empty() ||
         (text.size() >= suffix.size() &&
          memequal(text.data() + (text.size() - suffix.size()), suffix.data(),
                   suffix.size()) == 0);
}
bool EqualsIgnoreCase(std::wstring_view piece1, std::wstring_view piece2);
bool StartsWithIgnoreCase(std::wstring_view text, std::wstring_view prefix);
bool EndsWithIgnoreCase(std::wstring_view text, std::wstring_view suffix);

} // namespace bela

#endif
