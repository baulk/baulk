//
#include <bela/terminal.hpp>
#include <bela/str_cat.hpp>
#include <bela/io.hpp>
#include <algorithm>
#include <set>

struct LanguageConcept {
  std::wstring interpreter;
  std::wstring language;
};

void gen() {
  std::vector<LanguageConcept> languages = {
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

  std::sort(languages.begin(), languages.end(), [](const LanguageConcept &a, const LanguageConcept &b) -> bool {
    //
    return a.interpreter.compare(b.interpreter) < 0;
  });
  std::wstring s = LR"(struct Language {
  const std::wstring_view interpreter;
  const std::wstring_view language;
};
constexpr Language languages[] = {
)";
  for (const auto &i : languages) {
    bela::StrAppend(&s, L"    {L\"", i.interpreter, L"\", L\"", i.language, L"\"},\n");
  }
  bela::StrAppend(&s, L"};");
  bela::error_code ec;
  if (!bela::io::WriteText(s, L"./shabang.gen.inc", ec)) {
    bela::FPrintF(stderr, L"unable gen shabang.gen.inc: %s", ec.message);
    return;
  }
  bela::FPrintF(stderr, L"gen ./shabang.gen.inc success\n");
}

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

std::wstring_view LanguagesByInterpreter(const std::wstring_view ie) {
  auto max = std::size(languages);
  size_t min = 0;
  while (min <= max) {
    auto mid = min + (max - min) / 2;
    if (languages[mid].interpreter.compare(ie) >= 0) {
      max = mid - 1;
      continue;
    }
    min = mid + 1;
  }
  if (min < std::size(languages) && languages[min].interpreter == ie) {
    return languages[min].language;
  }
  return L"";
}

int wmain() {
  //
  gen();

  constexpr const std::wstring_view shebangs[] = {L"perl", L"deno", L"pwsh", L"sh", L"zsh", L"ash", L"jack"};
  for (auto p : shebangs) {
    bela::FPrintF(stderr, L"%s --> [%s]\n", p, LanguagesByInterpreter(p));
  }
  return 0;
}