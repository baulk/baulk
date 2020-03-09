#include <cstdio>
#include <cstdlib>
#include <clocale>

int wmain(int argc, wchar_t **argv) {
  // ----
  _wsetlocale(LC_ALL, L"");
  for (int i = 0; i < argc; i++) {
    wprintf(L"%d: [%s]\n", i, argv[i]);
  }
  return 0;
}