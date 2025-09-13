///
#include "hazelinc.hpp"
#include <bela/strip.hpp>
#include <bela/ascii.hpp>
#include <bela/str_split.hpp>
#include <bela/terminal.hpp>

namespace hazel {

struct Language {
  const std::wstring_view interpreter;
  const std::wstring_view language;
};
constexpr Language languages[] = {
    {.interpreter = L"M2", .language = L"Macaulay2"},
    {.interpreter = L"Rscript", .language = L"R"},
    {.interpreter = L"apl", .language = L"APL"},
    {.interpreter = L"aplx", .language = L"APL"},
    {.interpreter = L"ash", .language = L"Shell"},
    {.interpreter = L"asy", .language = L"Asymptote"},
    {.interpreter = L"awk", .language = L"Awk"},
    {.interpreter = L"bash", .language = L"Shell"},
    {.interpreter = L"bigloo", .language = L"Scheme"},
    {.interpreter = L"boolector", .language = L"SMT"},
    {.interpreter = L"ccl", .language = L"Common Lisp"},
    {.interpreter = L"chakra", .language = L"JavaScript"},
    {.interpreter = L"chicken", .language = L"Scheme"},
    {.interpreter = L"clisp", .language = L"Common Lisp"},
    {.interpreter = L"coffee", .language = L"CoffeeScript"},
    {.interpreter = L"cperl", .language = L"Perl"},
    {.interpreter = L"crystal", .language = L"Crystal"},
    {.interpreter = L"csh", .language = L"Tcsh"},
    {.interpreter = L"csi", .language = L"Scheme"},
    {.interpreter = L"cvc4", .language = L"SMT"},
    {.interpreter = L"cwl-runner", .language = L"Common Workflow Language"},
    {.interpreter = L"d8", .language = L"JavaScript"},
    {.interpreter = L"dafny", .language = L"Dafny"},
    {.interpreter = L"dart", .language = L"Dart"},
    {.interpreter = L"dash", .language = L"Shell"},
    {.interpreter = L"deno", .language = L"TypeScript"},
    {.interpreter = L"dtrace", .language = L"DTrace"},
    {.interpreter = L"dyalog", .language = L"APL"},
    {.interpreter = L"ecl", .language = L"Common Lisp"},
    {.interpreter = L"elixir", .language = L"Elixir"},
    {.interpreter = L"escript", .language = L"Erlang"},
    {.interpreter = L"fish", .language = L"fish"},
    {.interpreter = L"gawk", .language = L"Awk"},
    {.interpreter = L"gerbv", .language = L"Gerber Image"},
    {.interpreter = L"gerbview", .language = L"Gerber Image"},
    {.interpreter = L"gjs", .language = L"JavaScript"},
    {.interpreter = L"gn", .language = L"GN"},
    {.interpreter = L"gnuplot", .language = L"Gnuplot"},
    {.interpreter = L"gosh", .language = L"Scheme"},
    {.interpreter = L"groovy", .language = L"Groovy"},
    {.interpreter = L"gsed", .language = L"sed"},
    {.interpreter = L"guile", .language = L"Scheme"},
    {.interpreter = L"hy", .language = L"Hy"},
    {.interpreter = L"instantfpc", .language = L"Pascal"},
    {.interpreter = L"io", .language = L"Io"},
    {.interpreter = L"ioke", .language = L"Ioke"},
    {.interpreter = L"jconsole", .language = L"J"},
    {.interpreter = L"jolie", .language = L"Jolie"},
    {.interpreter = L"jruby", .language = L"Ruby"},
    {.interpreter = L"js", .language = L"JavaScript"},
    {.interpreter = L"julia", .language = L"Julia"},
    {.interpreter = L"ksh", .language = L"Shell"},
    {.interpreter = L"lisp", .language = L"Common Lisp"},
    {.interpreter = L"lsl", .language = L"LSL"},
    {.interpreter = L"lua", .language = L"Lua"},
    {.interpreter = L"macruby", .language = L"Ruby"},
    {.interpreter = L"make", .language = L"Makefile"},
    {.interpreter = L"makeinfo", .language = L"Texinfo"},
    {.interpreter = L"mathsat5", .language = L"SMT"},
    {.interpreter = L"mawk", .language = L"Awk"},
    {.interpreter = L"minised", .language = L"sed"},
    {.interpreter = L"mksh", .language = L"Shell"},
    {.interpreter = L"mmi", .language = L"Mercury"},
    {.interpreter = L"moon", .language = L"MoonScript"},
    {.interpreter = L"nawk", .language = L"Awk"},
    {.interpreter = L"newlisp", .language = L"NewLisp"},
    {.interpreter = L"nextflow", .language = L"Nextflow"},
    {.interpreter = L"node", .language = L"JavaScript"},
    {.interpreter = L"nodejs", .language = L"JavaScript"},
    {.interpreter = L"nush", .language = L"Nu"},
    {.interpreter = L"ocaml", .language = L"OCaml"},
    {.interpreter = L"ocamlrun", .language = L"OCaml"},
    {.interpreter = L"ocamlscript", .language = L"OCaml"},
    {.interpreter = L"openrc-run", .language = L"OpenRC runscript"},
    {.interpreter = L"opensmt", .language = L"SMT"},
    {.interpreter = L"osascript", .language = L"AppleScript"},
    {.interpreter = L"parrot", .language = L"Parrot Assembly"},
    {.interpreter = L"pdksh", .language = L"Shell"},
    {.interpreter = L"perl", .language = L"Perl"},
    {.interpreter = L"perl6", .language = L"Raku"},
    {.interpreter = L"php", .language = L"PHP"},
    {.interpreter = L"picolisp", .language = L"PicoLisp"},
    {.interpreter = L"pike", .language = L"Pike"},
    {.interpreter = L"pil", .language = L"PicoLisp"},
    {.interpreter = L"pwsh", .language = L"PowerShell"},
    {.interpreter = L"python", .language = L"Python"},
    {.interpreter = L"python2", .language = L"Python"},
    {.interpreter = L"python3", .language = L"Python"},
    {.interpreter = L"qjs", .language = L"JavaScript"},
    {.interpreter = L"qmake", .language = L"QMake"},
    {.interpreter = L"r6rs", .language = L"Scheme"},
    {.interpreter = L"racket", .language = L"Racket"},
    {.interpreter = L"rake", .language = L"Ruby"},
    {.interpreter = L"raku", .language = L"Raku"},
    {.interpreter = L"rakudo", .language = L"Raku"},
    {.interpreter = L"rbx", .language = L"Ruby"},
    {.interpreter = L"rc", .language = L"Shell"},
    {.interpreter = L"regina", .language = L"REXX"},
    {.interpreter = L"rexx", .language = L"REXX"},
    {.interpreter = L"rhino", .language = L"JavaScript"},
    {.interpreter = L"ruby", .language = L"Ruby"},
    {.interpreter = L"rune", .language = L"E"},
    {.interpreter = L"runghc", .language = L"Haskell"},
    {.interpreter = L"runhaskell", .language = L"Haskell"},
    {.interpreter = L"runhugs", .language = L"Haskell"},
    {.interpreter = L"sbcl", .language = L"Common Lisp"},
    {.interpreter = L"scala", .language = L"Scala"},
    {.interpreter = L"scheme", .language = L"Scheme"},
    {.interpreter = L"sclang", .language = L"SuperCollider"},
    {.interpreter = L"scsynth", .language = L"SuperCollider"},
    {.interpreter = L"sed", .language = L"sed"},
    {.interpreter = L"sh", .language = L"Shell"},
    {.interpreter = L"smt-rat", .language = L"SMT"},
    {.interpreter = L"smtinterpol", .language = L"SMT"},
    {.interpreter = L"ssed", .language = L"sed"},
    {.interpreter = L"stp", .language = L"SMT"},
    {.interpreter = L"swipl", .language = L"Prolog"},
    {.interpreter = L"tcc", .language = L"C"},
    {.interpreter = L"tclsh", .language = L"Tcl"},
    {.interpreter = L"tcsh", .language = L"Tcsh"},
    {.interpreter = L"ts-node", .language = L"TypeScript"},
    {.interpreter = L"v8", .language = L"JavaScript"},
    {.interpreter = L"v8-shell", .language = L"JavaScript"},
    {.interpreter = L"verit", .language = L"SMT"},
    {.interpreter = L"wish", .language = L"Tcl"},
    {.interpreter = L"yap", .language = L"Prolog"},
    {.interpreter = L"yices2", .language = L"SMT"},
    {.interpreter = L"z3", .language = L"SMT"},
    {.interpreter = L"zsh", .language = L"Shell"},
};

std::wstring_view LanguagesByInterpreter(const std::wstring_view ie) {
  int max = static_cast<int>(std::size(languages) - 1);
  int min = 0;
  while (min <= max) {
    auto mid = min + ((max - min) / 2);
    if (languages[mid].interpreter.compare(ie) >= 0) {
      max = mid - 1;
      continue;
    }
    min = mid + 1;
  }
  if (static_cast<size_t>(min) < std::size(languages) && languages[min].interpreter == ie) {
    return languages[min].language;
  }
  return L"";
}

namespace internal {
bool LookupShebang(const std::wstring_view line, hazel_result &hr) {
  constexpr const std::wstring_view prefix = L"#!";
  if (!bela::StartsWith(line, prefix)) {
    return false;
  }
  auto sline = bela::StripAsciiWhitespace(line.substr(2));
  std::vector<std::wstring_view> sv = bela::StrSplit(sline, bela::ByAnyChar(L"\r\n\t "), bela::SkipEmpty());
  if (sv.empty()) {
    return false;
  }
  std::wstring interpreter;
  if (bela::StrContains(sv[0], L"env")) {
    if (sv.size() > 1) {
      interpreter = sv[1];
    }
  } else {
    std::vector<std::wstring_view> spv = bela::StrSplit(sv[0], bela::ByChar('/'), bela::SkipEmpty());
    if (spv.empty()) {
      return false;
    }
    interpreter = spv.back();
  }
  if (interpreter == L"sh") {
    /// TODO
  }
  if (interpreter.empty()) {
    return false;
  }
  auto ln = LanguagesByInterpreter(interpreter);
  hr.append(L"Interpreter", std::move(interpreter));
  if (!ln.empty()) {
    hr.append(L"Language", ln);
  }
  return true;
}
} // namespace internal

} // namespace hazel