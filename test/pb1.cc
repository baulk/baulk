#include <bela/picker.hpp>
#include <bela/comutils.hpp>
#include <bela/terminal.hpp>
#include <ShObjIdl.h>
#include <ShlObj_core.h>
#include <thread>

class dot_global_initializer {
public:
  dot_global_initializer() {
    if (FAILED(::CoInitialize(nullptr))) {
      ::MessageBoxW(nullptr, L"CoInitialize() failed", L"COM initialize failed", MB_OK | MB_ICONERROR);
      ExitProcess(1);
    }
  }
  dot_global_initializer(const dot_global_initializer &) = delete;
  dot_global_initializer &operator=(const dot_global_initializer &) = delete;
  ~dot_global_initializer() { ::CoUninitialize(); }

private:
};

int wmain() {
  dot_global_initializer di;
  bela::comptr<IOperationsProgressDialog> bar;
  if (CoCreateInstance(CLSID_ProgressDialog, nullptr, CLSCTX_INPROC_SERVER, IID_IOperationsProgressDialog,
                       (void **)&bar) != S_OK) {
    auto ec = bela::make_system_error_code(L"CoCreateInstance ");
    bela::FPrintF(stderr, L"error %s\n", ec);
    return 1;
  }
  // if (bar->SetOperation(SPACTION_COPYING) != S_OK) {
  //   auto ec = bela::make_system_error_code(L"SetOperation ");
  //   bela::FPrintF(stderr, L"error %s\n", ec);
  //   // return 1;
  // }
  if (bar->StartProgressDialog(nullptr, OPPROGDLG_DEFAULT | OPPROGDLG_ALLOWUNDO) != S_OK) {
    auto ec = bela::make_system_error_code(L"StartProgressDialog ");
    bela::FPrintF(stderr, L"error %s\n", ec);
    return 1;
  }
  constexpr auto gb = 1024ull * 1024 * 1024;
  for (int i = 0; i < 200; i++) {
    bar->UpdateProgress(i, 200, gb / 200, gb, 1, 200);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }
  bar->StopProgressDialog();
  return 0;
}