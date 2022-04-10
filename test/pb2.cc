#include <bela/picker.hpp>
#include <bela/comutils.hpp>
#include <bela/terminal.hpp>
#include <bela/str_cat.hpp>
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
  bela::comptr<IProgressDialog> bar;
  if (CoCreateInstance(CLSID_ProgressDialog, nullptr, CLSCTX_INPROC_SERVER, IID_IProgressDialog, (void **)&bar) !=
      S_OK) {
    auto ec = bela::make_system_error_code(L"CoCreateInstance ");
    bela::FPrintF(stderr, L"error %s\n", ec);
    return 1;
  }
  bar->SetTitle(L"Extract baulk.tar.gz");
  // bar->SetLine(-1, L"extracting", true, nullptr);
  if (bar->StartProgressDialog(nullptr, nullptr, PROGDLG_AUTOTIME, nullptr) != S_OK) {
    auto ec = bela::make_system_error_code(L"StartProgressDialog ");
    bela::FPrintF(stderr, L"error %s\n", ec);
    return 1;
  }
  constexpr auto gb = 1024ull * 1024 * 1024;
  for (int i = 0; i < 200; i++) {
    auto s = bela::StringCat(L"Extract baulk.tar.gz ", i);
    bar->SetLine(2, s.data(), true, nullptr);
    bar->SetProgress64(i * (gb / 200), gb);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }
  bar->StopProgressDialog();
  return 0;
}