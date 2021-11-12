#ifndef BAULK_BRAND_HPP
#define BAULK_BRAND_HPP
#include <bela/base.hpp>
#include <bela/codecvt.hpp>
#include <bela/str_join.hpp>
#include <bela/terminal.hpp>
#include <bela/win32.hpp>

namespace baulk::brand {
namespace brand_internal {
// L"\x1b[48;5;39m", // 6
// L"\x1b[48;5;33m", // 7
// L"\x1b[48;5;33m", // 7
// L"\x1b[48;5;32m", // 8
constexpr const std::wstring_view colorTable[] = {
    L"\x1b[48;2;109;195;255m", // 5
    L"\x1b[48;2;85;185;255m",  // 6
    L"\x1b[48;2;61;175;255m",  // 7
    L"\x1b[48;2;36;165;255m",  // 8
};
constexpr std::wstring_view space = L"                ";

inline std::wstring registry_string(HKEY hKey, std::wstring_view subKey, std::wstring_view valName) {
  HKEY hCurrent = nullptr;
  if (RegOpenKeyW(hKey, subKey.data(), &hCurrent) != ERROR_SUCCESS) {
    return L"";
  }
  auto closer = bela::finally([&] { RegCloseKey(hCurrent); });
  wchar_t buffer[4096] = {0};
  DWORD dwSize = sizeof(buffer);
  DWORD dwType = 0;
  auto p = reinterpret_cast<LPBYTE>(buffer);
  if (auto status = RegQueryValueExW(hCurrent, valName.data(), nullptr, &dwType, p, &dwSize) == ERROR_SUCCESS &&
                    dwType == REG_SZ) {
    return std::wstring(buffer, wcsnlen(buffer, 4096));
  }
  return L"";
}

} // namespace brand_internal

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

class Detector;
class Render {
public:
  Render() = default;
  std::wstring Draw() {
    std::wstring text;
    text.resize(4096);
    for (size_t i = 0; i < 8; i++) {
      text.append(L" ")
          .append(brand_internal::colorTable[0])
          .append(brand_internal::space)
          .append(L"\x1b[48;5;15m  ")
          .append(brand_internal::colorTable[2])
          .append(brand_internal::space)
          .append(L"\x1b[0m");
      end_meta_line(text);
    }
    text.append(L" ")
        .append(L"\x1b[48;5;15m")
        .append(brand_internal::space)
        .append(L"  ")
        .append(brand_internal::space)
        .append(L"\x1b[0m");
    end_meta_line(text);
    for (size_t i = 0; i < 8; i++) {
      text.append(L" ")
          .append(brand_internal::colorTable[1])
          .append(brand_internal::space)
          .append(L"\x1b[48;5;15m  ")
          .append(brand_internal::colorTable[3])
          .append(brand_internal::space)
          .append(L"\x1b[0m");
      end_meta_line(text);
    }
    for (size_t i = 17; i < lines.size(); i++) {
      text.append(L" ").append(brand_internal::space).append(L"  ").append(brand_internal::space);
      end_meta_line(text);
    }
    return text;
  }

private:
  friend class Detector;
  meta_lines lines;
  bela::windows_version version;
  size_t index{0};
  void append_meta_line(std::wstring_view tab, std::wstring &&content) {
    lines.emplace_back(std::pair<std::wstring, std::wstring>(std::wstring(tab), std::move(content)));
  }
  void end_meta_line(std::wstring &text) {
    text.append(L"    ");
    if (index < lines.size()) {
      const auto &ml = lines[index];
      if (!ml.first.empty()) {
        text.append(L"\x1b[38;2;209;40;40m") // RGB(209,40,40)
            .append(ml.first)
            .append(L":\x1b[0m ");
      }
      text.append(ml.second);
    }
    text.append(L"\x1b[0m\n");
    index++;
  }
};

class Detector {
public:
  Detector() = default;
  bool Execute(bela::error_code &ec);
  void Swap(Render &render) const;
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

void Detector::Swap(Render &render) const {
  auto version = bela::windows::version();
  render.version = version;
  auto edition = brand_internal::registry_string(HKEY_LOCAL_MACHINE, LR"(SOFTWARE\Microsoft\Windows NT\CurrentVersion)",
                                                 L"EditionID");
  render.append_meta_line(L"OS", bela::StringCat(L"Windows 11 ", edition));
  render.append_meta_line(L"Kernel", bela::StringCat(version.major, L".", version.minor, L".", version.build));
  render.append_meta_line(L"CPU", processor());

  // Resolution
  std::vector<std::wstring> resolution;
  for (const auto &m : monitors) {
    resolution.emplace_back(m());
  }
  render.append_meta_line(L"Resolution", bela::StrJoin(resolution, L", "));

  std::wstring color_line1;
  std::wstring color_line2;
  for (int i = 0; i < 8; i++) {
    bela::StrAppend(&color_line1, L"\x1b[3", i, L"m\x1b[4", i, L"m    ");
    bela::StrAppend(&color_line2, L"\x1b[38;5;", i, L"m\x1b[48;5;", i, L"m    ");
  }
  color_line1.append(L"\x1b[0m");
  color_line2.append(L"\x1b[0m");
  // insert color end
  render.append_meta_line(L"", std::move(color_line1));
  render.append_meta_line(L"", std::move(color_line2));
}

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
  processor.brand = brand_internal::registry_string(
      HKEY_LOCAL_MACHINE, LR"(HARDWARE\DESCRIPTION\System\CentralProcessor\0)", L"ProcessorNameString");
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