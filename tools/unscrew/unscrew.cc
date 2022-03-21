#include "unscrew.hpp"
#include <CommCtrl.h>
#include <Objbase.h>

namespace baulk {

bool Executor::Execute(bela::error_code &ec) {
  bela::comptr<IProgressDialog> bar;
  if (CoCreateInstance(CLSID_ProgressDialog, nullptr, CLSCTX_INPROC_SERVER, IID_IProgressDialog, (void **)&bar) !=
      S_OK) {
    auto ec = bela::make_system_error_code(L"CoCreateInstance ");
    return false;
  }
  auto closer = bela::finally([&] { bar->StopProgressDialog(); });
  for (const auto &archive_file : archive_files) {
    auto e = MakeExtractor(archive_file, dest, opts, ec);
    if (!e) {
      return false;
    }
    if (!e->Extract(bar.get(), ec)) {
      return false;
    }
  }
  return true;
}

bool Executor::ParseArgv(bela::error_code &ec) {
  //

  return true;
}

} // namespace baulk

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

// Main
int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE, _In_ LPWSTR, _In_ int nCmdShow) {
  dot_global_initializer di;
  HeapSetInformation(NULL, HeapEnableTerminationOnCorruption, NULL, 0);
  INITCOMMONCONTROLSEX info = {sizeof(INITCOMMONCONTROLSEX),
                               ICC_TREEVIEW_CLASSES | ICC_COOL_CLASSES | ICC_LISTVIEW_CLASSES};
  InitCommonControlsEx(&info);

  return 0;
}