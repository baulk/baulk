///
#include <bela/escapeargv.hpp>
#include <bela/terminal.hpp>

//
int wmain(int argc, wchar_t **argv) {
  bela::EscapeArgv ea;
  ea.Assign(argv[0]);
  for (int i = 1; i < argc; i++) {
    ea.Append(argv[i]);
  }
  bela::FPrintF(stderr, L"%s\n", ea.sv());
  bela::EscapeArgv ea2(L"zzzz", L"", L"vvv ssdss", L"-D=\"JJJJJ sb\"");
  bela::FPrintF(stderr, L"%s\n", ea2.sv());
  return 0;
}