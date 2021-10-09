//
#include <baulk/fsmutex.hpp>
#include <bela/terminal.hpp>
#include <mutex>

int wmain() {
  bela::error_code ec;
  auto mtx = baulk::MakeFsMutex(L"abc.pid", ec);
  if (!mtx) {
    bela::FPrintF(stderr, L"PID file %s [%d]\n", ec.message, ec.code);
    return 1;
  }
  bela::FPrintF(stderr, L"lock success\n");
  using namespace std::chrono_literals;
  std::this_thread::sleep_for(20s);
  return 0;
}