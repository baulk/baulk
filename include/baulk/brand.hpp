#ifndef BAULK_BRAND_HPP
#define BAULK_BRAND_HPP
#include <bela/base.hpp>
#include <bela/codecvt.hpp>
#include <bela/terminal.hpp>

namespace baulk::brand {

struct processor {
  std::wstring brand;
  int cores{0};
  int logical_cores{0};
  std::wstring operator()() const { return bela::StringCat(brand, L" ", cores, L"/", logical_cores, L" processor"); }
};

struct monitor {
  uint32_t width{0};
  uint32_t height{0};
  int frequency{0};   // 60Hz
  int pixels{0};      // dpi
  int orientation{0}; // 0° 90° 180°
  // Resolution:
  std::wstring operator()() const {
    if (orientation != 0) {
      return bela::StringCat(width, L"x", height, L" ", frequency, L"Hz <LogPixels:", pixels, L" Orientation: ",
                             orientation, L"°>");
    }
    return bela::StringCat(width, L"x", height, L" ", frequency, L"Hz <LogPixels:", pixels, L">");
  }
};

struct memory_status {
  uint64_t total{0};
  uint64_t avail{0};
  uint32_t mem_load{0};
};

using meta_line = std::pair<std::wstring, std::wstring>;
using meta_lines = std::vector<meta_line>;

class Detector {
public:
  Detector() = default;
  bool Execute(bela::error_code &ec);
  const auto &Processor() const { return processor; }
  const auto &Monitors() const { return monitors; }
  const auto &Graphics() const { return graphics; }
  const auto &MemStatus() const { return mem_status; }

private:
  bool detect_processor(bela::error_code &ec);
  bool detect_monitors(bela::error_code &ec);
  bool detect_graphics(bela::error_code &ec);
  baulk::brand::processor processor;
  std::vector<monitor> monitors;
  std::vector<std::wstring> graphics;
  memory_status mem_status;
};

inline bool Detector::Execute(bela::error_code &ec) {
  if (!detect_processor(ec)) {
    return false;
  }
  if (!detect_monitors(ec)) {
    return false;
  }
  if (!detect_graphics(ec)) {
    return false;
  }
  MEMORYSTATUSEX ms{0};
  ms.dwLength = sizeof(MEMORYSTATUSEX);
  GlobalMemoryStatusEx(&ms);
  mem_status.mem_load = ms.dwMemoryLoad;
  mem_status.avail = ms.ullAvailPhys;
  mem_status.total = ms.ullTotalPhys;
  return true;
}

inline DWORD count_set_bits(ULONG_PTR bitMask) {
  DWORD LSHIFT = sizeof(ULONG_PTR) * 8 - 1;
  DWORD bitSetCount = 0;
  ULONG_PTR bitTest = (ULONG_PTR)1 << LSHIFT;
  DWORD i;

  for (i = 0; i <= LSHIFT; ++i) {
    bitSetCount += ((bitMask & bitTest) ? 1 : 0);
    bitTest /= 2;
  }
  return bitSetCount;
}

inline bool Detector::detect_processor(bela::error_code &ec) {
  [&]() -> bool {
    PSYSTEM_LOGICAL_PROCESSOR_INFORMATION buffer{nullptr};
    DWORD length{0};
    int cores{0};
    int logical_cores{0};
    GetLogicalProcessorInformation(buffer, &length);
    if (auto e = GetLastError(); e != ERROR_INSUFFICIENT_BUFFER) {
      ec = bela::make_system_error_code(L"GetLogicalProcessorInformation");
      return false;
    }
    if (buffer = reinterpret_cast<PSYSTEM_LOGICAL_PROCESSOR_INFORMATION>(malloc(length)); buffer == nullptr) {
      return false;
    }
    auto closer = bela::finally([&] { free(buffer); });
    if (GetLogicalProcessorInformation(buffer, &length) != TRUE) {
      ec = bela::make_system_error_code(L"GetLogicalProcessorInformation");
      return false;
    }
    auto n = length / sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION);
    for (size_t i = 0; i < n; i++) {
      if (buffer[i].Relationship == RelationProcessorCore) {
        cores++;
        logical_cores += count_set_bits(buffer[i].ProcessorMask);
      }
    }
    processor.cores = cores;
    processor.logical_cores = logical_cores;
    return true;
  }();

  constexpr std::wstring_view zeroProcessor = L"HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0";
  HKEY hKey = nullptr;
  if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, zeroProcessor.data(), 0, KEY_READ, &hKey) != ERROR_SUCCESS) {
    ec = bela::make_system_error_code(L"RegOpenKeyEx");
    return false;
  }
  auto closer = bela::finally([&] { RegCloseKey(hKey); });
  wchar_t processor_brand[256] = {0}; // CPU brand < 64
  DWORD dwSize = sizeof(processor_brand);
  DWORD type = 0;
  if (RegQueryValueExW(hKey, L"ProcessorNameString", nullptr, &type, reinterpret_cast<LPBYTE>(processor_brand),
                       &dwSize) != ERROR_SUCCESS) {
    ec = bela::make_system_error_code(L"RegQueryValueExW");
    return false;
  }
  if (type != REG_SZ) {
    ec = bela::make_error_code(bela::ErrGeneral, L"ProcessorNameString not REG_SZ: ", type);
    return false;
  }
  processor.brand.assign(processor_brand);
  return true;
}

inline bool Detector::detect_monitors(bela::error_code &ec) {
  auto n = GetSystemMetrics(SM_CMONITORS);
  for (int i = 0; i < n; i++) {
    DISPLAY_DEVICEW device{0};
    device.cb = sizeof(DISPLAY_DEVICEW);
    if (EnumDisplayDevicesW(NULL, i, &device, 0) != TRUE) {
      auto ec = bela::make_system_error_code();
      bela::FPrintF(stderr, L"EnumDisplayDevicesW error: %s\n", ec);
      continue;
    }
    DEVMODEW dm;
    dm.dmSize = sizeof(dm);
    dm.dmDriverExtra = 0;
    if (EnumDisplaySettingsExW(device.DeviceName, ENUM_CURRENT_SETTINGS, &dm, 0) != TRUE) {
      continue;
    }
    monitors.emplace_back(monitor{.width = dm.dmPelsWidth,
                                  .height = dm.dmPelsHeight,
                                  .frequency = static_cast<int>(dm.dmDisplayFrequency),
                                  .pixels = dm.dmLogPixels});
  }
  return true;
}

inline bool Detector::detect_graphics(bela::error_code &ec) {
  IDXGIFactory1 *factory{nullptr};
  IDXGIAdapter1 *adapter{nullptr};
  if (auto hr = CreateDXGIFactory1(__uuidof(IDXGIFactory1), reinterpret_cast<void **>(&factory)); FAILED(hr)) {
    ec = bela::make_system_error_code(L"CreateDXGIFactory1");
    return false;
  }
  auto closer = bela::finally([&] { factory->Release(); });
  UINT Adapter = 0;
  while (factory->EnumAdapters1(Adapter, &adapter) != DXGI_ERROR_NOT_FOUND) {
    Adapter++;
    DXGI_ADAPTER_DESC1 desc1;
    if (adapter->GetDesc1(&desc1) != S_OK) {
      ec = bela::make_system_error_code(L"GetDesc1");
      continue;
    }
    if ((desc1.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) != 0) {
      // Microsoft Basic Render Driver
      continue;
    }
    UINT displayNum = 0;
    IDXGIOutput *out = nullptr;
    while (adapter->EnumOutputs(displayNum, &out) != DXGI_ERROR_NOT_FOUND) {
      displayNum++;
      DXGI_OUTPUT_DESC desc;
      if (out->GetDesc(&desc) != S_OK) {
        continue;
      }
      graphics.emplace_back(desc.DeviceName);
    }
  }
  return true;
}

} // namespace baulk::brand

#endif