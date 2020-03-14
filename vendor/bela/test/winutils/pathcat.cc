///
#include <bela/path.hpp>
#include <bela/stdwriter.hpp>

int wmain() {
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
  //
  auto a1 = bela::PathAbsolute(L"C:\\Windows\\System32\\drivers\\..\\.\\IME");
  bela::FPrintF(stderr, L"PathAbsolute: %s\n", a1);
  auto a2 = bela::PathAbsolute(L"..\\..\\clangbuilder\\bin");
  bela::FPrintF(stderr, L"PathAbsolute: %s\n", a2);
  auto a3 = bela::PathAbsolute(L"\\\\?\\C:\\Windows/System32\\..\\notepad.exe");
  bela::FPrintF(stderr, L"PathAbsolute: %s\n", a3);
  return 0;
}