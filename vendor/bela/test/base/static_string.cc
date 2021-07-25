#include <bela/static_string.hpp>
#include <bela/terminal.hpp>
#include <cstdio>

int main() {
  bela::static_string<4096> exe;
  bela::FPrintF(stderr, L"%zu %zu max_size: %zu  sv: %s\n", exe.size(), exe.capacity(), exe.max_size(), exe.subview());
  //
  return 0;
}