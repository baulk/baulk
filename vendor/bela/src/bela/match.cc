///
#include <bela/match.hpp>
#include <bela/memutil.hpp>

namespace bela {

bool EqualsIgnoreCase(std::wstring_view piece1, std::wstring_view piece2) {
  return (piece1.size() == piece2.size() &&
          strings_internal::memcasecmp(piece1.data(), piece2.data(),
                                       piece1.size()) == 0);
}
bool StartsWithIgnoreCase(std::wstring_view text, std::wstring_view prefix) {
  return (text.size() >= prefix.size()) &&
         EqualsIgnoreCase(text.substr(0, prefix.size()), prefix);
}
bool EndsWithIgnoreCase(std::wstring_view text, std::wstring_view suffix) {
  return (text.size() >= suffix.size()) &&
         EqualsIgnoreCase(text.substr(text.size() - suffix.size()), suffix);
}
} // namespace bela
