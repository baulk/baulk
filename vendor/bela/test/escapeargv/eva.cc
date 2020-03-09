#include <cstdio>
#include "../../include/bela/escapeargv.hpp"
using EscapeArgvA = bela::basic_escape_argv<char>;

int main(int argc, char const *argv[]) {
  /* code */
  EscapeArgvA ea;
  ea.Assign(argv[0]);
  for (int i = 1; i < argc; i++) {
    ea.Append(argv[i]);
  }
  fprintf(stderr, "%s\n", ea.data());

  EscapeArgvA ea2("zzzz", "", "vvv ssdss", "-D=\"JJJJJ sb\"");
  ea2.AppendNoEscape("jacks ome");
  fprintf(stderr, "%s\n", ea2.data());
  return 0;
}
