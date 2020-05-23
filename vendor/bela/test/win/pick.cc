#include <bela/picker.hpp>
#include <bela/terminal.hpp>

class dotcom_global_initializer {
public:
  dotcom_global_initializer() {
    auto hr = CoInitialize(NULL);
    if (FAILED(hr)) {
      bela::FPrintF(stderr, L"initialize dotcom error: 0x%08x\n", hr);
      exit(1);
    }
  }
  ~dotcom_global_initializer() { CoUninitialize(); }
};

int wmain() {
  dotcom_global_initializer dot;
  bela::FPrintF(stderr, L"dotcom initialized.\n");
  auto f = bela::FolderPicker(nullptr, nullptr);
  if (!f) {
    auto ec = bela::make_system_error_code();
    bela::FPrintF(stderr, L"Picker Folder error: %s\n", ec.message);
    return 1;
  }
  bela::FPrintF(stderr, L"Folder is: %s\n", *f);
  return 0;
}