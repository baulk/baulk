//
#include "../example/vfsenv.hpp"
#include <bela/terminal.hpp>

int wmain() {
  bela::FPrintF(stderr, L"%v\n", example::PathSearcher::Instance().JoinEtc(L"wsudo.pos.json"));
  return 0;
}