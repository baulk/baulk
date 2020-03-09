////
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iterator>
#include <limits>
#include <memory>
#include <bela/str_split.hpp>

namespace bela {
// This GenericFind() template function encapsulates the finding algorithm
// shared between the ByString and ByAnyChar delimiters. The FindPolicy
// template parameter allows each delimiter to customize the actual find
// function to use and the length of the found delimiter. For example, the
// Literal delimiter will ultimately use std::wstring_view::find(), and the
// AnyOf delimiter will use std::wstring_view::find_first_of().
template <typename FindPolicy>
std::wstring_view GenericFind(std::wstring_view text,
                              std::wstring_view delimiter, size_t pos,
                              FindPolicy find_policy) {
  if (delimiter.empty() && text.length() > 0) {
    // Special case for empty std::string delimiters: always return a
    // zero-length std::wstring_view referring to the item at position 1 past
    // pos.
    return std::wstring_view(text.data() + pos + 1, 0);
  }
  size_t found_pos = std::wstring_view::npos;
  std::wstring_view found(text.data() + text.size(),
                          0); // By default, not found
  found_pos = find_policy.Find(text, delimiter, pos);
  if (found_pos != std::wstring_view::npos) {
    found = std::wstring_view(text.data() + found_pos,
                              find_policy.Length(delimiter));
  }
  return found;
}

// Finds using std::wstring_view::find(), therefore the length of the found
// delimiter is delimiter.length().
struct LiteralPolicy {
  size_t Find(std::wstring_view text, std::wstring_view delimiter, size_t pos) {
    return text.find(delimiter, pos);
  }
  size_t Length(std::wstring_view delimiter) { return delimiter.length(); }
};

// Finds using std::wstring_view::find_first_of(), therefore the length of the
// found delimiter is 1.
struct AnyOfPolicy {
  size_t Find(std::wstring_view text, std::wstring_view delimiter, size_t pos) {
    return text.find_first_of(delimiter, pos);
  }
  size_t Length(std::wstring_view /* delimiter */) { return 1; }
};
ByString::ByString(std::wstring_view sp) : delimiter_(sp) {}

std::wstring_view ByString::Find(std::wstring_view text, size_t pos) const {
  if (delimiter_.length() == 1) {
    // Much faster to call find on a single character than on an
    // std::wstring_view.
    size_t found_pos = text.find(delimiter_[0], pos);
    if (found_pos == std::wstring_view::npos)
      return std::wstring_view(text.data() + text.size(), 0);
    return text.substr(found_pos, 1);
  }
  return GenericFind(text, delimiter_, pos, LiteralPolicy());
}

//
// ByChar
//

std::wstring_view ByChar::Find(std::wstring_view text, size_t pos) const {
  size_t found_pos = text.find(c_, pos);
  if (found_pos == std::wstring_view::npos)
    return std::wstring_view(text.data() + text.size(), 0);
  return text.substr(found_pos, 1);
}

//
// ByAnyChar
//

ByAnyChar::ByAnyChar(std::wstring_view sp) : delimiters_(sp) {}

std::wstring_view ByAnyChar::Find(std::wstring_view text, size_t pos) const {
  return GenericFind(text, delimiters_, pos, AnyOfPolicy());
}

//
// ByLength
//
ByLength::ByLength(ptrdiff_t length) : length_(length) {
  //
}

std::wstring_view ByLength::Find(std::wstring_view text, size_t pos) const {
  pos = std::min(pos, text.size()); // truncate `pos`
  std::wstring_view substr = text.substr(pos);
  // If the std::string is shorter than the chunk size we say we
  // "can't find the delimiter" so this will be the last chunk.
  if (substr.length() <= static_cast<size_t>(length_))
    return std::wstring_view(text.data() + text.size(), 0);

  return std::wstring_view(substr.data() + length_, 0);
}

} // namespace bela
