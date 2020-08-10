//////////
#include <bela/simulator.hpp>
#include <bela/terminal.hpp>

namespace bela {
namespace env {
std::wstring_view resovle_shell_name(std::wstring_view s, size_t &off);
}
} // namespace bela

int wmain(int argc, wchar_t **argv) {
  const wchar_t *sss[] = {L"{HOME}", L"{HOME,,,,}", L"HOME----", L"{}", L"HOME$"};
  for (auto s : sss) {
    size_t off = 0;
    auto k = bela::env::resovle_shell_name(s, off);
    bela::FPrintF(stderr, L"%s --> [%s] off: %d\n", s, k, off);
  }
  bela::env::Simulator simulator;
  simulator.InitializeCleanupEnv();
  simulator.AddBashCompatible(argc, argv);
  simulator.PutEnv(L"JACK=ROSE");

  const wchar_t *svv[] = {                                                 //
                          L"SystemRoot ${SystemRoot}, $ who $JACK ?$$$ |", //
                          L"System $|--- $ ???",
                          L"System $|--- $ ???${",
                          L"------->${}", // ------->
                          L"App Argv0: $0 HOME: $HOME",
                          L"cmdline: $@"};
  for (auto s : svv) {
    bela::FPrintF(stderr, L"%s --> [%s]\n", s, simulator.ExpandEnv(s));
  }
  auto path = bela::AppendEnv(L"Path", L"C:\\Program Files\\7-Zip", L"C:\\MSYS2");
  bela::FPrintF(stderr, L"Path: %s\n", path);
  return 0;
}