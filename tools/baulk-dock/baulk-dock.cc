// baulk-dock entrypoint
#include "app.hpp"

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
int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE, _In_ LPWSTR, _In_ int) {
  dot_global_initializer di;
  HeapSetInformation(NULL, HeapEnableTerminationOnCorruption, NULL, 0);
  INITCOMMONCONTROLSEX info = {sizeof(INITCOMMONCONTROLSEX),
                               ICC_TREEVIEW_CLASSES | ICC_COOL_CLASSES | ICC_LISTVIEW_CLASSES};
  InitCommonControlsEx(&info);
  baulk::dock::MainWindow window(hInstance);
  MSG msg;
  window.InitializeWindow();
  window.ShowWindow(SW_SHOW);
  window.UpdateWindow();
  while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
    TranslateMessage(&msg);
    DispatchMessageW(&msg);
  }
  return 0;
}
