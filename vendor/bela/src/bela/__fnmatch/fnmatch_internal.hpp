///
#ifndef BELA_FNMATCH_INTERNAL_HPP
#define BELA_FNMATCH_INTERNAL_HPP
#include <bela/fnmatch.hpp>
#include <bela/ascii.hpp>
#include <bela/codecvt.hpp>
#include <wctype.h>

namespace bela::fnmatch_internal {
constexpr int END = 0;
constexpr int UNMATCHABLE = -2;
constexpr int BRACKET = -3;
constexpr int QUESTION = -4;
constexpr int STAR = -5;

constexpr bool ascii_isalpha_u16_template(wchar_t c) { return true; }

constexpr bool ascii_isalpha_u8_template(char8_t c) { return true; }

constexpr bool same_character_matched(char16_t wc, std::u16string_view sv) {
  constexpr struct {
    std::u16string_view sv;
    decltype(ascii_isalpha_u16_template) *fn;
  } matchers[] = {
      {u"alnum", bela::ascii_isalnum}, {u"alpha", bela::ascii_isalpha}, {u"blank", bela::ascii_isblank},
      {u"cntrl", bela::ascii_iscntrl}, {u"digit", bela::ascii_isdigit}, {u"graph", bela::ascii_isgraph},
      {u"lower", bela::ascii_islower}, {u"print", bela::ascii_isprint}, {u"punct", bela::ascii_ispunct},
      {u"space", bela::ascii_isspace}, {u"upper", bela::ascii_isupper}, {u"xdigit", bela::ascii_isxdigit},
  };
  for (const auto &m : matchers) {
    if (m.sv == sv) {
      return m.fn(wc);
    }
  }
  return false;
}

constexpr bool same_character_matched(char8_t wc, std::u8string_view sv) {
  constexpr struct {
    std::u8string_view sv;
    decltype(ascii_isalpha_u8_template) *fn;
  } matchers[] = {
      {u8"alnum", bela::ascii_isalnum}, {u8"alpha", bela::ascii_isalpha}, {u8"blank", bela::ascii_isblank},
      {u8"cntrl", bela::ascii_iscntrl}, {u8"digit", bela::ascii_isdigit}, {u8"graph", bela::ascii_isgraph},
      {u8"lower", bela::ascii_islower}, {u8"print", bela::ascii_isprint}, {u8"punct", bela::ascii_ispunct},
      {u8"space", bela::ascii_isspace}, {u8"upper", bela::ascii_isupper}, {u8"xdigit", bela::ascii_isxdigit},
  };
  for (const auto &m : matchers) {
    if (m.sv == sv) {
      return m.fn(wc);
    }
  }
  return false;
}

} // namespace bela::fnmatch_internal

#endif