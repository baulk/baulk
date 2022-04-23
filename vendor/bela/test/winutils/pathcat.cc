///
#include <bela/path.hpp>
#include <bela/terminal.hpp>
#include <filesystem>

void BaseName() {
  bela::FPrintF(stderr, L"\x1b[33mBaseName\x1b[0m\n");
  constexpr std::wstring_view bv[] = {L"C:",
                                      L"C://jackson//jack//",
                                      L"jackson/zzz/j",
                                      L"C:/",
                                      L"w/cpp/string/basic_string/find_first_of",
                                      L"bela\\build\\test\\pathcat\\pathcat_test.exe"};
  for (auto p : bv) {
    bela::FPrintF(stderr, L"BaseName: [\x1b[33m%s\x1b[0m] -->[\x1b[32m%s\x1b[0m]\n", p, bela::BaseName(p));
  }
}

void DirName() {
  constexpr std::wstring_view dirs[] = {L"C:\\User\\Jack\\helloworld.txt", L"C:\\User\\Jack\\helloworld.txt\\\\\\",
                                        L"C:\\User\\Jack\\\\helloworld.txt\\\\\\",
                                        L"C:\\User\\Jack\\helloworld.txt\\\\\\.\\\\", L"C:\\User\\Jack\\"};
  for (const auto d : dirs) {
    std::error_code e;
    bela::FPrintF(stderr, L"DirName: [\x1b[33m%s\x1b[0m] -->[\x1b[32m%s\x1b[0m] [fs: %v]\n", d, bela::DirName(d),
                  std::filesystem::path(d).parent_path());
  }
}

void BaseCat() {
  bela::FPrintF(stderr, L"\x1b[33mBase cat\x1b[0m\n");
  auto p = bela::PathCat(L"\\\\?\\C:\\Windows/System32", L"drivers/etc", L"hosts");
  bela::FPrintF(stderr, L"PathCat: %s\n", p);
  auto p2 = bela::PathCat(L"C:\\Windows/System32", L"drivers/../..");
  bela::FPrintF(stderr, L"PathCat: %s\n", p2);
  auto p3 = bela::PathCat(L"Windows/System32", L"drivers/./././.\\.\\etc");
  bela::FPrintF(stderr, L"PathCat: %s\n", p3);
  auto p4 = bela::PathCat(L".", L"test/pathcat/./pathcat_test.exe");
  bela::FPrintF(stderr, L"PathCat: %s\n", p4);
  auto p5 = bela::PathCat(L"C:\\Windows\\System32\\drivers\\..\\.\\IME");
  bela::FPrintF(stderr, L"PathCat: %s\n", p5);
  auto p6 = bela::PathCat(L"\\\\server\\short\\Windows\\System32\\drivers\\..\\.\\IME");
  bela::FPrintF(stderr, L"PathCat: %s\n", p6);
  auto p7 = bela::PathCat(L"\\\\server\\short\\C:\\Windows\\System32\\drivers\\..\\.\\IME");
  bela::FPrintF(stderr, L"PathCat: %s\n", p7);
  auto p8 = bela::PathCat(L"cmd");
  bela::FPrintF(stderr, L"PathCat: %s\n", p8);
}

void JoinPath() {
  bela::FPrintF(stderr, L"\x1b[33mJoinPath\x1b[0m\n");
  auto p = bela::JoinPath(L"\\\\?\\C:\\Windows/System32", L"drivers/etc", L"hosts");
  bela::FPrintF(stderr, L"JoinPath: %s\n", p);
  auto p2 = bela::JoinPath(L"C:\\Windows/System32", L"drivers/../..");
  bela::FPrintF(stderr, L"JoinPath: %s\n", p2);
  auto p3 = bela::JoinPath(L"Windows/System32", L"drivers/./././.\\.\\etc");
  bela::FPrintF(stderr, L"JoinPath: %s\n", p3);
  auto p4 = bela::JoinPath(L".", L"test/pathcat/./pathcat_test.exe");
  bela::FPrintF(stderr, L"JoinPath: %s\n", p4);
  auto p5 = bela::JoinPath(L"C:\\Windows\\System32\\drivers\\..\\.\\IME");
  bela::FPrintF(stderr, L"JoinPath: %s\n", p5);
  auto p6 = bela::JoinPath(L"\\\\server\\short\\Windows\\System32\\drivers\\..\\.\\IME");
  bela::FPrintF(stderr, L"JoinPath: %s\n", p6);
  auto p7 = bela::JoinPath(L"\\\\server\\short\\C:\\Windows\\System32\\drivers\\..\\.\\IME");
  bela::FPrintF(stderr, L"JoinPath: %s\n", p7);
  auto p8 = bela::JoinPath(L"cmd");
  bela::FPrintF(stderr, L"JoinPath: %s\n", p8);
}

void FullPath() {
  constexpr std::wstring_view paths[] = {
      // ------------
      L"C:\\Windows\\System32\\drivers\\..\\.\\IME",
      L"..\\..\\clangbuilder\\bin",
      L"\\\\?\\C:\\Windows/System32\\..\\notepad.exe",
      L"/.",
      L"~/Desktop/Code/cl.exe",
      L"cmd",
      L".",
      L"C:\\User\\Jack\\helloworld.txt",
      L"C:\\User\\Jack\\helloworld.txt\\\\\\",
      L"C:\\User\\Jack\\\\helloworld.txt\\\\\\",
      L"C:\\User\\Jack\\helloworld.txt\\\\\\.\\\\",
      L"C:\\User\\Jack\\"
      //
  };
  bela::FPrintF(stderr, L"\x1b[33mFullPath\x1b[0m\n");
  for (const auto p : paths) {
    bela::FPrintF(stderr, L"FullPath: [\x1b[33m%s\x1b[0m] --> \x1b[32m%s\x1b[0m\n", p, bela::FullPath(p));
    std::error_code e;
    bela::FPrintF(stderr, L"fs abs: [\x1b[33m%s\x1b[0m] --> \x1b[32m%s\x1b[0m\n", p, std::filesystem::absolute(p, e));
  }
}

int wmain() {
  BaseName();
  DirName();
  BaseCat();
  JoinPath();
  FullPath();
  return 0;
}