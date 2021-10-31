#include "window.hpp"
#include <CommCtrl.h>
#include <Objbase.h>

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
  baulk::unscrew::MainWindow win;

  if (!win.MakeWindow(hInstance, L"Extract baulk-win64-4.0.zip")) {
    return 0;
  }

  ShowWindow(win.Window(), nCmdShow);

  // Run the message loop.

  MSG msg = {0};
  while (GetMessageW(&msg, NULL, 0, 0) == TRUE) {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }

  return 0;
}