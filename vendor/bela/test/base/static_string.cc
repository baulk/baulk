#include <bela/static_string.hpp>
#include <cstdio>

int main() {
  bela::static_string<4096> exe;
  fprintf(stderr, "%zu %zu max_size: %zu\n", exe.size(), exe.capacity(), exe.max_size());
  //
  return 0;
}