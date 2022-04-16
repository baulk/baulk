#include <bela/terminal.hpp>
#include <bela/fnmatch.hpp>

void fnmatch_assert(std::string_view p, std::string_view s) {
  if (!bela::FnMatch(p, s)) {
    bela::FPrintF(stderr, L"[%v] \x1b[31mmust\x1b[0m match [%v]\n", p, s);
    return;
  }
  bela::FPrintF(stderr, L"[%v] \x1b[33mmatched\x1b[0m [%v]\n", p, s);
}

void round0() {
  fnmatch_assert("", "");
  fnmatch_assert("*", "");
  fnmatch_assert("*", "foo");
  fnmatch_assert("*", "bar");
  fnmatch_assert("*", "*");
  fnmatch_assert("**", "f");
  fnmatch_assert("**", "foo.txt");
  fnmatch_assert("*.*", "foo.txt");
  fnmatch_assert("foo*.txt", "foobar.txt");
  fnmatch_assert("foo.txt", "foo.txt");
  fnmatch_assert("foo\\.txt", "foo.txt");
}

void fnmatch_assert(std::wstring_view p, std::wstring_view s) {
  if (!bela::FnMatch(p, s)) {
    bela::FPrintF(stderr, L"[%v] \x1b[31mmust\x1b[0m match [%v]\n", p, s);
    return;
  }
  bela::FPrintF(stderr, L"[%v] \x1b[33mmatched\x1b[0m [%v]\n", p, s);
}

void round1() {
  fnmatch_assert(L"", L"");
  fnmatch_assert(L"*", L"");
  fnmatch_assert(L"*", L"foo");
  fnmatch_assert(L"*", L"bar");
  fnmatch_assert(L"*", L"*");
  fnmatch_assert(L"**", L"f");
  fnmatch_assert(L"**", L"foo.txt");
  fnmatch_assert(L"*.*", L"foo.txt");
  fnmatch_assert(L"foo*.txt", L"foobar.txt");
  fnmatch_assert(L"foo.txt", L"foo.txt");
  fnmatch_assert(L"foo\\.txt", L"foo.txt");
}

void TestWildcard() {
  constexpr struct {
    std::string_view pattern;
    std::string_view input;
    int flags;
    bool want;
  } cases[]{
      {"*", "", 0, true},    {"*", "foo", 0, true},  {"*", "*", 0, true},
      {"*", "   ", 0, true}, {"*", ".foo", 0, true}, {"*", "わたし", 0, true},
  };
  for (const auto &c : cases) {
    auto got = bela::FnMatch(c.pattern, c.input, c.flags);
    if (got != c.want) {
      bela::FPrintF(stderr, L"Testcase failed: FnMatch('%s', '%s', %d) should be %v not %v\n", c.pattern, c.input,
                    c.flags, c.want, got);
    }
  }
}

void TestRange() {
  constexpr std::string_view azPat = "[a-z]";
  constexpr struct {
    std::string_view pattern;
    std::string_view input;
    int flags;
    bool want;
  } cases[]{
      // Should match a single character inside its range
      {azPat, "a", 0, true},
      {azPat, "q", 0, true},
      {azPat, "z", 0, true},
      {"[わ]", "わ", 0, true},

      // Should not match characters outside its range
      {azPat, "-", 0, false},
      {azPat, " ", 0, false},
      {azPat, "D", 0, false},
      {azPat, "é", 0, false},

      // Should only match one character
      {azPat, "ab", 0, false},
      {azPat, "", 0, false},

      // Should not consume more of the pattern than necessary
      {"[a-z]foo", "afoo", 0, true},

      // Should match '-' if it is the first/last character or is
      // backslash escaped
      {"[-az]", "-", 0, true},
      {"[-az]", "a", 0, true},
      {"[-az]", "b", 0, false},
      {"[az-]", "-", 0, true},
      {"[a\\-z]", "-", 0, true},
      {"[a\\-z]", "b", 0, false},

      // ignore '\\' when FNM_NOESCAPE is given
      {"[a\\-z]", "\\", bela::fnmatch::NoEscape, true},
      {"[a\\-z]", "-", bela::fnmatch::NoEscape, false},

      // Should be negated if starting with ^ or !"
      {"[^a-z]", "a", 0, false},
      {"[!a-z]", "b", 0, false},
      {"[!a-z]", "é", 0, true},
      {"[!a-z]", "わ", 0, true},

      // Still match '-' if following the negation character
      {"[^-az]", "-", 0, false},
      {"[^-az]", "b", 0, true},

      // Should support multiple characters/ranges
      {"[abc]", "a", 0, true},
      {"[abc]", "c", 0, true},
      {"[abc]", "d", 0, false},
      {"[a-cg-z]", "c", 0, true},
      {"[a-cg-z]", "h", 0, true},
      {"[a-cg-z]", "d", 0, false},

      // Should not match '/' when flags is FNM_PATHNAME
      {"[abc/def]", "/", 0, true},
      {"[abc/def]", "/", bela::fnmatch::PathName, false},
      {"[.-0]", "/", 0, true}, // The range [.-0] includes /
      {"[.-0]", "/", bela::fnmatch::PathName, false},

      // Should normally be case-sensitive
      {"[a-z]", "A", 0, false},
      {"[A-Z]", "a", 0, false},
      // Except when FNM_CASEFOLD is given
      {"[a-z]", "A", bela::fnmatch::CaseFold, true},
      {"[A-Z]", "a", bela::fnmatch::CaseFold, true},
  };
  for (const auto &c : cases) {
    auto got = bela::FnMatch(c.pattern, c.input, c.flags);
    if (got != c.want) {
      bela::FPrintF(stderr, L"Testcase failed: FnMatch('%s', '%s', %d) should be %v not %v\n", c.pattern, c.input,
                    c.flags, c.want, got);
      continue;
    }
    bela::FPrintF(stderr, L"Testcase OK: FnMatch('%s', '%s', %d) \x1b[33m%v\x1b[0m\n", c.pattern, c.input, c.flags,
                  got);
  }
}

int wmain() {
  round0();
  round1();
  TestWildcard();
  TestRange();
  return 0;
}