#include "indicators.hpp"

void NoMaximum(const wchar_t *filename) {
  baulk::ProgressBar bar;
  bar.FileName(filename);
  bar.Execute();
  uint64_t total = 0;
  for (int i = 0; i < 2048; i++) {
    std::this_thread::sleep_for(std::chrono::microseconds(10));
    total += 1024 * 512;
    bar.Update(total);
  }
  bar.MarkCompleted();
  bar.Finish();
}

void Maximum(const wchar_t *filename) {
  baulk::ProgressBar bar;
  bar.FileName(filename);
  bar.Maximum(1024 * 1024 * 1024);
  bar.Execute();
  uint64_t total = 0;
  for (int i = 0; i < 2048; i++) {
    std::this_thread::sleep_for(std::chrono::microseconds(10));
    total += 1024 * 512;
    bar.Update(total);
  }
  bar.MarkCompleted();
  bar.Finish();
}

int wmain(int argc, wchar_t **argv) {
  if (argc < 2) {
    bela::FPrintF(stderr, L"usage: %s filename\n", argv[0]);
    return 1;
  }
  Maximum(argv[1]);
  NoMaximum(argv[1]);
  return 0;
}