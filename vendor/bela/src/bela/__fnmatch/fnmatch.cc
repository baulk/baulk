///
#include <bela/strings.hpp>
#include <bela/unicode.hpp>
#include "fnmatch_internal.hpp"

namespace bela {
namespace fnmatch_internal {
template <typename C>
requires bela::character<C>
bool rangematch(std::basic_string_view<C, std::char_traits<C>> &pattern, char32_t rune, int flags) {
  if (pattern.empty()) {
    return false;
  }
  auto casefold = (flags & fnmatch::CaseFold) != 0;
  auto noescape = (flags & fnmatch::NoEscape) != 0;
  if (casefold) {
    rune = bela::unicode::ToLower(rune);
  }
  bool negate{false};
  bool matched{false};
  if (auto c = pattern[0]; c == '^' || c == '!') {
    negate = true;
    pattern.remove_prefix(1);
  }
  for (; !matched && pattern.size() > 1 && pattern[0] != ']';) {
    auto c = bela::RuneNext(pattern);
    if (!noescape && c == '\\') {
      if (pattern.size() <= 1) {
        return false;
      }
      c = bela::RuneNext(pattern);
    }
    if (casefold) {
      c = bela::unicode::ToLower(c);
    }
    if (pattern.size() > 1 && pattern[0] == '-' && pattern[1] != ']') {
      pattern.remove_prefix(1);
      auto c2 = bela::RuneNext(pattern);
      if (!noescape && c2 == '\\') {
        if (pattern.empty()) {
          return false;
        }
        c2 = bela::RuneNext(pattern);
      }
      if (casefold) {
        c2 = bela::unicode::ToLower(c2);
      }
      // this really should be more intelligent, but it looks like
      // fnmatch.c does simple int comparisons, therefore we will as well
      if (c <= rune && rune <= c2) {
        matched = true;
      }
      continue;
    }
    if (c == rune) {
      matched = true;
    }
  }
  auto ok{false};
  for (; !ok && !pattern.empty();) {
    auto c = bela::RuneNext(pattern);
    if (c == '\\' && !pattern.empty()) {
      bela::RuneNext(pattern);
      continue;
    }
    if (c == ']') {
      ok = true;
    }
  }

  return ok && (matched != negate);
}

template <typename C>
requires bela::character<C>
bool FnMatchInternal(std::basic_string_view<C, std::char_traits<C>> pattern,
                     std::basic_string_view<C, std::char_traits<C>> s, int flags) {
  constexpr auto npos = std::basic_string_view<C, std::char_traits<C>>::npos;
  auto noescape = (flags & fnmatch::NoEscape) != 0;
  auto pathname = (flags & fnmatch::PathName) != 0;
  auto period = (flags & fnmatch::Period) != 0;
  auto leadingdir = (flags & fnmatch::LeadingDir) != 0;
  auto casefold = (flags & fnmatch::CaseFold) != 0;
  auto sAtStart = true;
  auto sLastAtStart = true;
  auto sLastSlash = false;
  char32_t sLastUnpacked = 0;
  auto unpack = [&]() -> char32_t {
    sLastSlash = (sLastUnpacked == '/');
    sLastUnpacked = bela::RuneNext(s);
    sLastAtStart = sAtStart;
    sAtStart = false;
    return sLastUnpacked;
  };
  for (; !pattern.empty();) {
    auto c = bela::RuneNext(pattern);
    switch (c) {
    case '?': {
      if (s.empty()) {
        return false;
      }
      auto sc = unpack();
      if (pathname && sc == '/') {
        return false;
      }
      if (period && sc == '.' && (sLastAtStart || (pathname && sLastSlash))) {
        return false;
      }
    } break;
    case '*': {
      // collapse multiple *'s
      // don't use unpackRune here, the only char we care to detect is ASCII
      if (!pattern.empty() && pattern[0] == '*') {
        pattern.remove_prefix(1);
      }
      if (period && s[0] == '.' && (sAtStart || (pathname && sLastUnpacked == '/'))) {
        return false;
      }
      // optimize for patterns with * at end or before /
      if (pattern.empty()) {
        if (pathname) {
          return leadingdir || s.find('/') == npos;
        }
        return true;
      }
      if (pathname && pattern[0] == '/') {
        auto pos = s.find('/');
        if (pos == npos) {
          return false;
        }
        // we already know our pattern and string have a /, skip past it
        s.remove_prefix(pos); // use unpackS here to maintain our bookkeeping state
        unpack();
        pattern.remove_prefix(1); // we know / is one byte long
        break;
      }
      // general case, recurse
      for (auto test = s; !test.empty(); bela::RuneNext(test)) {
        // I believe the (flags &^ FNM_PERIOD) is a bug when FNM_PATHNAME is specified
        // but this follows exactly from how fnmatch.c implements it
        if (FnMatchInternal(pattern, test, (flags & (~fnmatch::Period)))) {
          return true;
        }
        if (pathname && test[0] == '/') {
          break;
        }
      }
      return false;
    }
    case '[': {
      if (s.empty()) {
        return false;
      }
      if (pathname && s[0] == '/') {
        return false;
      }
      auto sc = unpack();
      if (!rangematch(pattern, sc, flags)) {
        return false;
      }
    } break;
    case '\\':
      if (!noescape) {
        if (!pattern.empty()) {
          c = bela::RuneNext(pattern);
        }
      }
      [[fallthrough]];
    default: {
      if (s.empty()) {
        return false;
      }
      auto sc = unpack();
      if (sc == c) {
        break;
      }
      if (casefold && unicode::ToLower(sc) == unicode::ToLower(c)) {
        break;
      }
      return false;
    }
    }
  }
  return s.empty() || (leadingdir && s[0] == '/');
}
} // namespace fnmatch_internal

bool FnMatch(std::u16string_view pattern, std::u16string_view text, int flags) {
  return fnmatch_internal::FnMatchInternal(pattern, text, flags);
}
bool FnMatch(std::u8string_view pattern, std::u8string_view text, int flags) {
  return fnmatch_internal::FnMatchInternal(pattern, text, flags);
}
} // namespace bela