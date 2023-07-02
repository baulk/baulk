#include <baulk/wmic.hpp>

class dotcom_global_initializer {
public:
  dotcom_global_initializer() {
    auto hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if (FAILED(hr)) {
      auto ec = bela::make_system_error_code();
      MessageBoxW(nullptr, ec.data(), L"CoInitialize", IDOK);
      exit(1);
    }
    auto result = CoInitializeSecurity(NULL,
                                       -1,                          // COM authentication
                                       NULL,                        // Authentication services
                                       NULL,                        // Reserved
                                       RPC_C_AUTHN_LEVEL_DEFAULT,   // Default authentication
                                       RPC_C_IMP_LEVEL_IMPERSONATE, // Default Impersonation
                                       NULL,                        // Authentication info
                                       EOAC_NONE,                   // Additional capabilities
                                       NULL                         // Reserved
    );

    if (FAILED(result)) {
      auto ec = bela::make_system_error_code();
      MessageBoxW(nullptr, ec.data(), L"CoInitializeSecurity", IDOK);
      exit(1);
    }
  }
  ~dotcom_global_initializer() { CoUninitialize(); }
};

int wmain() {
  dotcom_global_initializer di;
  bela::error_code ec;
  auto ds = baulk::wmic::search_disk_drives(ec);
  if (!ds) {
    bela::FPrintF(stderr, L"look %v\n", ec);
    return 1;
  }
  constexpr auto GiB = 1024 * 1024 * 1024ull;
  for (const auto &d : ds->drives) {
    bela::FPrintF(stderr, L"Disk: %s FriendlyName: %s Size: %0.2f GB (%s %s)\n", d.device_id, d.friendly_name,
                  double(d.size) / GiB, d.bus_type, baulk::wmic::disk_media_name(d.m));
  }
  return 0;
}