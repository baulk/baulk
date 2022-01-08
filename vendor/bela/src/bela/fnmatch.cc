// Origin fnmatch.cc copyright
/*
 * An implementation of what I call the "Sea of Stars" algorithm for
 * POSIX fnmatch(). The basic idea is that we factor the pattern into
 * a head component (which we match first and can reject without ever
 * measuring the length of the string), an optional tail component
 * (which only exists if the pattern contains at least one star), and
 * an optional "sea of stars", a set of star-separated components
 * between the head and tail. After the head and tail matches have
 * been removed from the input string, the components in the "sea of
 * stars" are matched sequentially by searching for their first
 * occurrence past the end of the previous match.
 *
 * - Rich Felker, April 2012
 */
// FnMatch
#include <bela/fnmatch.hpp>

namespace bela {
constexpr int END = 0;
constexpr int UNMATCHABLE = -2;
constexpr int BRACKET = -3;
constexpr int QUESTION = -4;
constexpr int STAR = -5;

constexpr bool same_character_matched(wint_t wc, std::u16string_view sv) {
  constexpr struct {
    std::u16string_view sv;
    decltype(iswalnum) *fn;
  } matchers[] = {
      {u"alnum", iswalnum}, {u"alpha", iswalpha}, {u"blank", iswblank}, {u"cntrl", iswcntrl},
      {u"digit", iswdigit}, {u"graph", iswgraph}, {u"lower", iswlower}, {u"print", iswprint},
      {u"punct", iswpunct}, {u"space", iswspace}, {u"upper", iswupper}, {u"xdigit", iswxdigit},
  };
  for (const auto &m : matchers) {
    if (m.sv == sv) {
      return m.fn(wc) != 0;
    }
  }
  return false;
} // namespace bela

// convert UTF-16 text to UTF-32
int CharNext(const char16_t *str, size_t n, size_t *step) {
  if (n == 0) {
    *step = 0;
    return 0;
  }
  char32_t ch = str[0];
  if (ch >= 0xD800 && ch <= 0xDBFF) {
    if (n < 2) {
      *step = 1;
      return -1;
    }
    *step = 2;
    char32_t ch2 = str[1];
    if (ch2 < 0xDC00 || ch2 > 0xDFFF) {
      *step = 1;
      return -1;
    }
    ch = ((ch - 0xD800) << 10) + (ch2 - 0xDC00) + 0x10000U;
    return ch;
  }
  *step = 1;
  return ch;
}

int CharUnicode(char32_t *c, const char16_t *str, size_t n) {
  if (n == 0) {
    return -1;
  }
  char32_t ch = str[0];
  if (ch >= 0xD800 && ch <= 0xDBFF) {
    if (n < 2) {
      return -1;
    }
    char32_t ch2 = str[1];
    if (ch2 < 0xDC00 || ch2 > 0xDFFF) {
      return -1;
    }
    *c = ((ch - 0xD800) << 10) + (ch2 - 0xDC00) + 0x10000U;
    return 2;
  }
  *c = ch;
  return 1;
}

static int PatternNext(const char16_t *pat, size_t m, size_t *step, int flags) {
  int esc = 0;
  if ((m == 0U) || (*pat == 0U)) {
    *step = 0;
    return END;
  }
  *step = 1;
  if (pat[0] == '\\' && (pat[1] != 0U) && (flags & fnmatch::NoEscape) == 0) {
    *step = 2;
    pat++;
    esc = 1;
    goto escaped;
  }
  if (pat[0] == '[') {
    size_t k = 1;
    if (k < m) {
      if (pat[k] == '^' || pat[k] == '!') {
        k++;
      }
    }
    if (k < m) {
      if (pat[k] == ']') {
        k++;
      }
    }
    for (; k < m && (pat[k] != 0U) && pat[k] != ']'; k++) {
      if (k + 1 < m && (pat[k + 1] != 0U) && pat[k] == '[' && (pat[k + 1] == ':' || pat[k + 1] == '.' || pat[k + 1] == '=')) {
        int z = pat[k + 1];
        k += 2;
        if (k < m && (pat[k] != 0U)) {
          k++;
        }
        while (k < m && (pat[k] != 0U) && (pat[k - 1] != z || pat[k] != ']')) {
          k++;
        }
        if (k == m || (pat[k] == 0U)) {
          break;
        }
      }
    }
    if (k == m || (pat[k] == 0U)) {
      *step = 1;
      return '[';
    }
    *step = k + 1;
    return BRACKET;
  }
  if (pat[0] == '*') {
    return STAR;
  }
  if (pat[0] == '?') {
    return QUESTION;
  }
escaped:
  if (pat[0] >= 0xD800 && pat[0] <= 0xDBFF) {
    char32_t ch = pat[0];
    if (m < 2) {
      *step = 0;
      return UNMATCHABLE;
    }
    char32_t ch2 = pat[1];
    ch = ((ch - 0xD800) << 10) + (ch2 - 0xDC00) + 0x10000U;
    *step = 2 + esc;
    return ch;
  }
  return pat[0];
}

static inline int CaseFold(int k) {
  int c = towupper(static_cast<wint_t>(k));
  return c == k ? towlower(static_cast<wint_t>(k)) : c;
}

static int MatchBracket(const char16_t *p, int k, int kfold) {
  char32_t wc;
  int inv = 0;
  p++;
  if (*p == '^' || *p == '!') {
    inv = 1;
    p++;
  }
  if (*p == ']') {
    if (k == ']') {
      return static_cast<int>(static_cast<int>(inv) == 0);
    }
    p++;
  } else if (*p == '-') {
    if (k == '-') {
      return static_cast<int>(static_cast<int>(inv) == 0);
    }
    p++;
  }
  wc = p[-1];
  for (; *p != ']'; p++) {
    if (p[0] == '-' && p[1] != ']') {
      char32_t wc2;
      int l = CharUnicode(&wc2, p + 1, 4);
      if (l < 0) {
        return 0;
      }
      if (wc <= wc2) {
        if ((unsigned)k - wc <= wc2 - wc || (unsigned)kfold - wc <= wc2 - wc) {
          return static_cast<int>(static_cast<int>(inv) == 0);
        }
      }
      p += l - 1;
      continue;
    }
    if (p[0] == '[' && (p[1] == ':' || p[1] == '.' || p[1] == '=')) {
      const char16_t *p0 = p + 2;
      int z = p[1];
      p += 3;
      while (p[-1] != z || p[0] != ']') {
        p++;
      }
      auto svlen = p - 1 - p0;
      if (z == ':' && svlen < 16) {
        std::u16string_view sv{p0, static_cast<size_t>(svlen)};
        if (same_character_matched(static_cast<wint_t>(k), sv) ||
            same_character_matched(static_cast<wint_t>(kfold), sv)) {
          return static_cast<int>(static_cast<int>(inv) == 0);
        }
      }
      continue;
    }
    if (*p < 128U) {
      wc = (unsigned char)*p;
    } else {
      int l = CharUnicode(&wc, p, 4);
      if (l < 0) {
        return 0;
      }
      p += l - 1;
    }
    if (wc == static_cast<char32_t>(k) || wc == static_cast<char32_t>(kfold)) {
      return static_cast<int>(static_cast<int>(inv) == 0);
    }
  }
  return inv;
}

static int FnMatchInternal(const char16_t *pat, size_t m, const char16_t *str, size_t n, int flags) {
  const char16_t *p;
  const char16_t *ptail;
  const char16_t *endpat;
  const char16_t *s;
  const char16_t *stail;
  const char16_t *endstr;
  size_t pinc;
  size_t sinc;
  size_t tailcnt = 0;
  int c;
  int k;
  int kfold;

  if ((flags & fnmatch::Period) != 0) {
    if (*str == '.' && *pat != '.') {
      return 1;
}
  }
  for (;;) {
    switch ((c = PatternNext(pat, m, &pinc, flags))) {
    case UNMATCHABLE:
      return 1;
    case STAR:
      pat++;
      m--;
      break;
    default:
      k = CharNext(str, n, &sinc);
      if (k <= 0) {
        return (c == END) ? 0 : 1;
      }
      str += sinc;
      n -= sinc;
      kfold = (flags & fnmatch::CaseFold) != 0 ? CaseFold(k) : k;
      if (c == BRACKET) {
        if (MatchBracket(pat, k, kfold) == 0) {
          return 1;
        }
      } else if (c != QUESTION && k != c && kfold != c) {
        return 1;
      }
      pat += pinc;
      m -= pinc;
      continue;
    }
    break;
  }

  endpat = pat + m;

  /* Find the last * in pat and count chars needed after it */
  for (p = ptail = pat; p < endpat; p += pinc) {
    switch (PatternNext(p, endpat - p, &pinc, flags)) {
    case UNMATCHABLE:
      return 1;
    case STAR:
      tailcnt = 0;
      ptail = p + 1;
      break;
    default:
      tailcnt++;
      break;
    }
  }

  /* Past this point we need not check for UNMATCHABLE in pat,
   * because all of pat has already been parsed once. */
  endstr = str + n;
  if (n < tailcnt) {
    return 1;
  }

  /* Find the final tailcnt chars of str, accounting for UTF-8.
   * On illegal sequences we may get it wrong, but in that case
   * we necessarily have a matching failure anyway. */
  for (s = endstr; s > str && (tailcnt != 0U); tailcnt--) {
    if (s[-1] < 128U || MB_CUR_MAX == 1) {
      s--;
      continue;
    }
    while ((unsigned char)*--s - 0x80U < 0x40 && s > str) {
    }
  }
  if (tailcnt != 0U) {
    return 1;
  }
  stail = s;

  /* Check that the pat and str tails match */
  p = ptail;
  for (;;) {
    c = PatternNext(p, endpat - p, &pinc, flags);
    p += pinc;
    if ((k = CharNext(s, endstr - s, &sinc)) <= 0) {
      if (c != END) {
        return 1;
      }
      break;
    }
    s += sinc;
    kfold = (flags & fnmatch::CaseFold) != 0 ? CaseFold(k) : k;
    if (c == BRACKET) {
      if (MatchBracket(p - pinc, k, kfold) == 0) {
        return 1;
      }
    } else if (c != QUESTION && k != c && kfold != c) {
      return 1;
    }
  }

  /* We're all done with the tails now, so throw them out */
  endstr = stail;
  endpat = ptail;

  /* Match pattern components until there are none left */
  while (pat < endpat) {
    p = pat;
    s = str;
    for (;;) {
      c = PatternNext(p, endpat - p, &pinc, flags);
      p += pinc;
      /* Encountering * completes/commits a component */
      if (c == STAR) {
        pat = p;
        str = s;
        break;
      }
      k = CharNext(s, endstr - s, &sinc);
      if (k == 0) {
        return 1;
      }
      kfold = (flags & fnmatch::CaseFold) != 0 ? CaseFold(k) : k;
      if (c == BRACKET) {
        if (MatchBracket(p - pinc, k, kfold) == 0) {
          break;
        }
      } else if (c != QUESTION && k != c && kfold != c) {
        break;
      }
      s += sinc;
    }
    if (c == STAR) {
      continue;
}
    /* If we failed, advance str, by 1 char if it's a valid
     * char, or past all invalid bytes otherwise. */
    k = CharNext(str, endstr - str, &sinc);
    if (k > 0) {
      str += sinc;
    } else {
      for (str++; CharNext(str, endstr - str, &sinc) < 0; str++) {
        /// empty
      }
    }
  }

  return 0;
}

int FnMatchInternal(std::u16string_view pattern, std::u16string_view text, int flags) {
  return FnMatchInternal(pattern.data(), pattern.size(), text.data(), text.size(), flags);
}

// Thanks https://github.com/bminor/musl/blob/master/src/regex/fnmatch.c
bool FnMatch(std::u16string_view pattern, std::u16string_view text, int flags) {
  if (pattern.empty() || text.empty()) {
    return false;
  }
  if ((flags & fnmatch::PathName) != 0) {
    // auto pos = text.find_first_of(u"\\/");
    // auto pat = pattern;
    // size_t inc = 0;
    // int c = 0;
    // for (;;) {
    //   if (c = PatternNext(pat.data(), pat.size(), &inc, flags); (c == END || c == '/')) {
    //     break;
    //   }
    //   pat.remove_prefix(inc);
    // }
    // auto p = pos == std::u16string_view::npos ? 0 : text[pos];
    // if (c != p && (pos == std::u16string_view::npos || (flags & fnmatch::LeadingDir) == 0)) {
    //   return false;
    // }
    // auto pattern_ = pat.substr(inc);
    // auto text_ = text.substr(pos + 1);
    // if (FnMatchInternal(pattern_, text_, flags) == 0) {
    //   return true;
    // }
    // return false;
  }
  if ((flags & fnmatch::LeadingDir) != 0) {
    if (auto pos = text.find_first_of(u"\\/"); pos != std::u16string_view::npos) {
      text.remove_suffix(text.size() - pos);
    }
  }
  return FnMatchInternal(pattern, text, flags) == 0;
}

inline std::u16string_view u16sv(std::wstring_view sv) {
  return std::u16string_view{reinterpret_cast<const char16_t *>(sv.data()), sv.size()};
}

// Thanks https://github.com/bminor/musl/blob/master/src/regex/fnmatch.c
bool FnMatch(std::wstring_view pattern, std::wstring_view text, int flags) {
  return FnMatch(u16sv(pattern), u16sv(text), flags);
}

} // namespace bela