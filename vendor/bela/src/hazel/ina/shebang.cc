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
    {L"M2", L"Macaulay2"},
    {L"Rscript", L"R"},
    {L"apl", L"APL"},
    {L"aplx", L"APL"},
    {L"ash", L"Shell"},
    {L"asy", L"Asymptote"},
    {L"awk", L"Awk"},
    {L"bash", L"Shell"},
    {L"bigloo", L"Scheme"},
    {L"boolector", L"SMT"},
    {L"ccl", L"Common Lisp"},
    {L"chakra", L"JavaScript"},
    {L"chicken", L"Scheme"},
    {L"clisp", L"Common Lisp"},
    {L"coffee", L"CoffeeScript"},
    {L"cperl", L"Perl"},
    {L"crystal", L"Crystal"},
    {L"csh", L"Tcsh"},
    {L"csi", L"Scheme"},
    {L"cvc4", L"SMT"},
    {L"cwl-runner", L"Common Workflow Language"},
    {L"d8", L"JavaScript"},
    {L"dafny", L"Dafny"},
    {L"dart", L"Dart"},
    {L"dash", L"Shell"},
    {L"deno", L"TypeScript"},
    {L"dtrace", L"DTrace"},
    {L"dyalog", L"APL"},
    {L"ecl", L"Common Lisp"},
    {L"elixir", L"Elixir"},
    {L"escript", L"Erlang"},
    {L"fish", L"fish"},
    {L"gawk", L"Awk"},
    {L"gerbv", L"Gerber Image"},
    {L"gerbview", L"Gerber Image"},
    {L"gjs", L"JavaScript"},
    {L"gn", L"GN"},
    {L"gnuplot", L"Gnuplot"},
    {L"gosh", L"Scheme"},
    {L"groovy", L"Groovy"},
    {L"gsed", L"sed"},
    {L"guile", L"Scheme"},
    {L"hy", L"Hy"},
    {L"instantfpc", L"Pascal"},
    {L"io", L"Io"},
    {L"ioke", L"Ioke"},
    {L"jconsole", L"J"},
    {L"jolie", L"Jolie"},
    {L"jruby", L"Ruby"},
    {L"js", L"JavaScript"},
    {L"julia", L"Julia"},
    {L"ksh", L"Shell"},
    {L"lisp", L"Common Lisp"},
    {L"lsl", L"LSL"},
    {L"lua", L"Lua"},
    {L"macruby", L"Ruby"},
    {L"make", L"Makefile"},
    {L"makeinfo", L"Texinfo"},
    {L"mathsat5", L"SMT"},
    {L"mawk", L"Awk"},
    {L"minised", L"sed"},
    {L"mksh", L"Shell"},
    {L"mmi", L"Mercury"},
    {L"moon", L"MoonScript"},
    {L"nawk", L"Awk"},
    {L"newlisp", L"NewLisp"},
    {L"nextflow", L"Nextflow"},
    {L"node", L"JavaScript"},
    {L"nodejs", L"JavaScript"},
    {L"nush", L"Nu"},
    {L"ocaml", L"OCaml"},
    {L"ocamlrun", L"OCaml"},
    {L"ocamlscript", L"OCaml"},
    {L"openrc-run", L"OpenRC runscript"},
    {L"opensmt", L"SMT"},
    {L"osascript", L"AppleScript"},
    {L"parrot", L"Parrot Assembly"},
    {L"pdksh", L"Shell"},
    {L"perl", L"Perl"},
    {L"perl6", L"Raku"},
    {L"php", L"PHP"},
    {L"picolisp", L"PicoLisp"},
    {L"pike", L"Pike"},
    {L"pil", L"PicoLisp"},
    {L"pwsh", L"PowerShell"},
    {L"python", L"Python"},
    {L"python2", L"Python"},
    {L"python3", L"Python"},
    {L"qjs", L"JavaScript"},
    {L"qmake", L"QMake"},
    {L"r6rs", L"Scheme"},
    {L"racket", L"Racket"},
    {L"rake", L"Ruby"},
    {L"raku", L"Raku"},
    {L"rakudo", L"Raku"},
    {L"rbx", L"Ruby"},
    {L"rc", L"Shell"},
    {L"regina", L"REXX"},
    {L"rexx", L"REXX"},
    {L"rhino", L"JavaScript"},
    {L"ruby", L"Ruby"},
    {L"rune", L"E"},
    {L"runghc", L"Haskell"},
    {L"runhaskell", L"Haskell"},
    {L"runhugs", L"Haskell"},
    {L"sbcl", L"Common Lisp"},
    {L"scala", L"Scala"},
    {L"scheme", L"Scheme"},
    {L"sclang", L"SuperCollider"},
    {L"scsynth", L"SuperCollider"},
    {L"sed", L"sed"},
    {L"sh", L"Shell"},
    {L"smt-rat", L"SMT"},
    {L"smtinterpol", L"SMT"},
    {L"ssed", L"sed"},
    {L"stp", L"SMT"},
    {L"swipl", L"Prolog"},
    {L"tcc", L"C"},
    {L"tclsh", L"Tcl"},
    {L"tcsh", L"Tcsh"},
    {L"ts-node", L"TypeScript"},
    {L"v8", L"JavaScript"},
    {L"v8-shell", L"JavaScript"},
    {L"verit", L"SMT"},
    {L"wish", L"Tcl"},
    {L"yap", L"Prolog"},
    {L"yices2", L"SMT"},
    {L"z3", L"SMT"},
    {L"zsh", L"Shell"},
};

const std::wstring_view LanguagesByInterpreter(const std::wstring_view ie) {
  int max = static_cast<int>(std::size(languages) - 1);
  int min = 0;
  while (min <= max) {
    auto mid = min + (max - min) / 2;
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