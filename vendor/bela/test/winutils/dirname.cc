///
#include <bela/path.hpp>
#include <bela/terminal.hpp>

int wmain() {
  constexpr std::wstring_view dirs[] = {
      L"C:\\User\\Jack\\helloworld.txt", L"C:\\User\\Jack\\helloworld.txt\\\\\\",
      L"C:\\User\\Jack\\\\helloworld.txt\\\\\\", L"C:\\User\\Jack\\helloworld.txt\\\\\\.\\\\",
      L"C:\\User\\Jack\\"};
  for (const auto d : dirs) {
    bela::FPrintF(stderr, L"%s--%s\n", d, bela::DirName(d));
  }
  return 0;
}