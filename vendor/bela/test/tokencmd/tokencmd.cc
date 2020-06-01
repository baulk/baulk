///
#include <bela/base.hpp>
#include <bela/terminal.hpp>
#include <bela/tokenizecmdline.hpp>
#include <clocale>

int TestTokenize() {
  auto cmd = L"ccc\\clang -c -DFOO=\"\"\"ABC\"\"\" x.cpp    ";
  bela::FPrintF(stderr, L"tokenize [%s]\n", cmd);
  bela::Tokenizer tokenizer;
  if (!tokenizer.Tokenize(cmd)) {
    bela::FPrintF(stderr, L"unable tokenize [%s]\n", cmd);
    return 1;
  }
  for (size_t i = 0; i < tokenizer.Argc(); i++) {
    bela::FPrintF(stderr, L"Got: [%s]\n", tokenizer.Argv()[i]);
  }
  return 0;
}

int wmain(int argc, wchar_t **argv) {
  _wsetlocale(LC_ALL, L"");
  TestTokenize();
  std::wstring_view cmdline = GetCommandLineW();
  bela::FPrintF(stderr, L"cmdline: [%s]\n", cmdline);
  bela::Tokenizer tokenizer;
  if (!tokenizer.Tokenize(cmdline)) {
    bela::FPrintF(stderr, L"unable tokenize [%s]\n", cmdline);
    return 1;
  }
  if (tokenizer.Argc() != (size_t)argc) {
    bela::FPrintF(stderr, L"command line number not match %d --> %d\n", tokenizer.Argc(), argc);
    return 1;
  }
  for (int i = 0; i < argc; i++) {
    bela::FPrintF(stderr, L"Need: [%s] Got: [%s]\n", argv[i], tokenizer.Argv()[i]);
  }
  return 0;
}