///
#include <bela/path.hpp>
#include <bela/stdwriter.hpp>

std::wstring FullPath(std::wstring_view p) {
  DWORD dwlen = GetFullPathNameW(p.data(), 0, nullptr, nullptr);
  std::wstring str;
  str.resize(dwlen);
  dwlen = GetFullPathNameW(p.data(), dwlen, str.data(), nullptr);
  str.resize(dwlen);
  return str;
}

void BaseNameParse() {
  constexpr std::wstring_view bv[] = {L"C:", L"C://jackson//jack//",
                                      L"jackson/zzz/j", L"C:/", L""};
  for (auto p : bv) {
    bela::FPrintF(stderr, L"BaseName: %s -->[%s]\n", p, bela::BaseName(p));
  }
}

void BaseCat() {
  bela::FPrintF(stderr, L"\x1b[33mBase cat\x1b[0m\n");
  auto p =
      bela::PathCat(L"\\\\?\\C:\\Windows/System32", L"drivers/etc", L"hosts");
  bela::FPrintF(stderr, L"PathCat: %s\n", p);
  auto p2 = bela::PathCat(L"C:\\Windows/System32", L"drivers/../..");
  bela::FPrintF(stderr, L"PathCat: %s\n", p2);
  auto p3 = bela::PathCat(L"Windows/System32", L"drivers/./././.\\.\\etc");
  bela::FPrintF(stderr, L"PathCat: %s\n", p3);
  auto p4 = bela::PathCat(L".", L"test/pathcat/./pathcat_test.exe");
  bela::FPrintF(stderr, L"PathCat: %s\n", p4);
  auto p5 = bela::PathCat(L"C:\\Windows\\System32\\drivers\\..\\.\\IME");
  bela::FPrintF(stderr, L"PathCat: %s\n", p5);
  auto p6 = bela::PathCat(
      L"\\\\server\\short\\Windows\\System32\\drivers\\..\\.\\IME");
  bela::FPrintF(stderr, L"PathCat: %s\n", p6);
  auto p7 = bela::PathCat(
      L"\\\\server\\short\\C:\\Windows\\System32\\drivers\\..\\.\\IME");
  bela::FPrintF(stderr, L"PathCat: %s\n", p7);
  auto p8 = bela::PathCat(L"cmd");
  bela::FPrintF(stderr, L"PathCat: %s\n", p8);
}

void AbsCat() {
  bela::FPrintF(stderr, L"\x1b[33mAbs cat\x1b[0m\n");
  auto p = bela::PathAbsoluteCat(L"\\\\?\\C:\\Windows/System32", L"drivers/etc",
                                 L"hosts");
  bela::FPrintF(stderr, L"PathAbsoluteCat: %s\n", p);
  auto p2 = bela::PathAbsoluteCat(L"C:\\Windows/System32", L"drivers/../..");
  bela::FPrintF(stderr, L"PathAbsoluteCat: %s\n", p2);
  auto p3 =
      bela::PathAbsoluteCat(L"Windows/System32", L"drivers/./././.\\.\\etc");
  bela::FPrintF(stderr, L"PathAbsoluteCat: %s\n", p3);
  auto p4 = bela::PathAbsoluteCat(L".", L"test/pathcat/./pathcat_test.exe");
  bela::FPrintF(stderr, L"PathAbsoluteCat: %s\n", p4);
  auto p5 =
      bela::PathAbsoluteCat(L"C:\\Windows\\System32\\drivers\\..\\.\\IME");
  bela::FPrintF(stderr, L"PathAbsoluteCat: %s\n", p5);
  auto p6 = bela::PathAbsoluteCat(
      L"\\\\server\\short\\Windows\\System32\\drivers\\..\\.\\IME");
  bela::FPrintF(stderr, L"PathAbsoluteCat: %s\n", p6);
  auto p7 = bela::PathAbsoluteCat(
      L"\\\\server\\short\\C:\\Windows\\System32\\drivers\\..\\.\\IME");
  bela::FPrintF(stderr, L"PathAbsoluteCat: %s\n", p7);
  auto p8 = bela::PathAbsoluteCat(L"cmd");
  bela::FPrintF(stderr, L"PathAbsoluteCat: %s\n", p8);
}

int wmain() {
  BaseNameParse();
  BaseCat();
  AbsCat();
  //
  auto a1 = bela::PathAbsolute(L"C:\\Windows\\System32\\drivers\\..\\.\\IME");
  bela::FPrintF(stderr, L"PathAbsolute: %s\n", a1);
  auto a2 = bela::PathAbsolute(L"..\\..\\clangbuilder\\bin");
  bela::FPrintF(stderr, L"PathAbsolute: %s\n", a2);
  auto a3 = bela::PathAbsolute(L"\\\\?\\C:\\Windows/System32\\..\\notepad.exe");
  bela::FPrintF(stderr, L"PathAbsolute: %s\n", a3);
  auto a4 = bela::PathAbsolute(L"/.");
  bela::FPrintF(stderr, L"PathAbsolute: %s\n", a4);
  auto a5 = FullPath(L"/.");
  bela::FPrintF(stderr, L"FullPath: %s\n", a5);
  auto a6 = bela::PathAbsolute(L"cmd");
  bela::FPrintF(stderr, L"PathAbsolute: %s\n", a6);
  return 0;
}