#ifndef BAULK_BRAND_HPP
#define BAULK_BRAND_HPP
#include <bela/base.hpp>
#include <bela/codecvt.hpp>
#include <bela/str_join.hpp>
#include <bela/terminal.hpp>
#include <bela/time.hpp>
#include <bela/win32.hpp>
#include <bela/env.hpp>
#include <dxgi.h>

namespace baulk::brand {
namespace brand_internal {
// L"\x1b[48;5;39m", // 6
// L"\x1b[48;5;33m", // 7
// L"\x1b[48;5;33m", // 7
// L"\x1b[48;5;32m", // 8
//
// L"\x1b[48;2;109;195;255m", // 5
// L"\x1b[48;2;85;185;255m",  // 6
// L"\x1b[48;2;61;175;255m",  // 7
// L"\x1b[48;2;36;165;255m",  // 8

#if defined(_M_X64)
constexpr std::wstring_view update_stack_subkey =
    L"SOFTWARE\\Microsoft\\Windows "
    L"NT\\CurrentVersion\\Update\\TargetingInfo\\Installed\\Windows.UpdateStackPackage.amd64";
#elif defined(_M_ARM64)
constexpr std::wstring_view update_stack_subkey =
    L"SOFTWARE\\Microsoft\\Windows "
    L"NT\\CurrentVersion\\Update\\TargetingInfo\\Installed\\Windows.UpdateStackPackage.arm64";
#else
constexpr std::wstring_view update_stack_subkey =
    L"SOFTWARE\\Microsoft\\Windows "
    L"NT\\CurrentVersion\\Update\\TargetingInfo\\Installed\\Windows.UpdateStackPackage.x86";
#endif

constexpr const std::wstring_view windows_colors[] = {
    L"\x1b[48;2;109;195;255m", // 5
    L"\x1b[48;2;61;175;255m",  // 7
    L"\x1b[48;2;61;175;255m",  // 7
    L"\x1b[48;2;36;165;255m",  // 8
};

constexpr const std::wstring_view border = L"      ";
constexpr const std::wstring_view padding = L"                ";
constexpr const std::wstring_view windows10_art_lines[] = {
    //
    L"                         ....::::", // 0
    L"                 ....::::::::::::", // 1
    L"        ....:::: ::::::::::::::::", // 2
    L"....:::::::::::: ::::::::::::::::", // 3
    L":::::::::::::::: ::::::::::::::::", // 4
    L":::::::::::::::: ::::::::::::::::", // 5
    L":::::::::::::::: ::::::::::::::::", // 6
    L":::::::::::::::: ::::::::::::::::", // 7
    L"................ ................", // 8
    L":::::::::::::::: ::::::::::::::::", // 9
    L":::::::::::::::: ::::::::::::::::", // 10
    L":::::::::::::::: ::::::::::::::::", // 11
    L"'''':::::::::::: ::::::::::::::::", // 12
    L"        '''':::: ::::::::::::::::", // 13
    L"                 ''''::::::::::::", // 14
    L"                         ''''::::", // 15
    L"                                 ", // 16
    L"                                 ", // 17
    L"                                 ", // 18
};

inline std::wstring_view pop_art_line(size_t i) {
  constexpr auto L = std::size(windows10_art_lines);
  if (i < L) {
    return windows10_art_lines[i];
  }
  return windows10_art_lines[L - 1];
}

constexpr auto TiB = 1024 * 1024 * 1024 * 1024ull;
constexpr auto GiB = 1024 * 1024 * 1024ull;
constexpr auto MiB = 1024 * 1024ull;

inline std::wstring TerminalName() {
  if (auto ws = bela::GetEnv(L"WT_SESSION"); !ws.empty()) {
    return L"Windows Terminal";
  }
  if (auto tm = bela::GetEnv(L"TERM_PROGRAM"); !tm.empty()) {
    return tm;
  }
  return L"conhost";
}

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
  std::wstring operator()() const { return bela::StringCat(brand, L" ", cores, L"C", logical_cores, L"T"); }
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
      return bela::StringCat(width, L"x", height, L" ", frequency, L"Hz <LogPixels: ", pixels, L" Orientation: ",
                             orientation, L"°>");
    }
    return bela::StringCat(width, L"x", height, L" ", frequency, L"Hz <LogPixels: ", pixels, L">");
  }
};

struct storage {
  std::wstring diskDevice;
  uint64_t total{0};
  uint64_t avail{0};
  uint8_t latter{0};
  std::wstring operator()() const {
    return bela::StringCat(diskDevice, L" ", (total - avail) / brand_internal::GiB, L"GB / ",
                           total / brand_internal::GiB, L"GB (", (total - avail) * 100 / total, L"%)");
  }
};

struct memory_status {
  uint64_t total{0};
  uint64_t avail{0};
  uint32_t load{0};
};

using meta_line = std::pair<std::wstring, std::wstring>;
using meta_lines = std::vector<meta_line>;

class Detector;
class Render {
public:
  Render() = default;
  std::wstring Draw() {
    if (version >= bela::windows::win11_21h2) {
      return DrawWindows11();
    }
    return DrawWindows10();
  }
  std::wstring DrawWindows11();
  std::wstring DrawWindows10();

private:
  friend class Detector;
  meta_lines lines;
  bela::windows_version version;
  size_t index{0};
  void append_meta_line(const std::wstring_view tab, std::wstring &&content) {
    lines.emplace_back(std::pair<std::wstring, std::wstring>(std::wstring(tab), std::move(content)));
  }
  void end_meta_line(std::wstring &text) {
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

inline std::wstring Render::DrawWindows10() {
  std::wstring text;
  text.resize(4096);
  auto maxlines = (std::max)(std::size(brand_internal::windows10_art_lines), lines.size());
  text.append(L" ")
      .append(brand_internal::pop_art_line(std::size(brand_internal::windows10_art_lines)))
      .append(brand_internal::border)
      .append(L"\x1b[38;2;255;215;0m")
      .append(bela::GetEnv(L"USERNAME"))
      .append(L"\x1b[0m@\x1b[38;2;0;191;255m")
      .append(bela::GetEnv(L"COMPUTERNAME"))
      .append(L"\x1b[0m\n");
  for (size_t i = 0; i < maxlines; i++) {
    text.append(L" \x1b[38;2;0;191;255m")
        .append(brand_internal::pop_art_line(i))
        .append(L"\x1b[0m")
        .append(brand_internal::border);
    end_meta_line(text);
  }
  return text;
}

inline std::wstring Render::DrawWindows11() {
  index = 0;
  std::wstring text;
  text.resize(4096);
  text.append(L" ")
      .append(brand_internal::padding)
      .append(L"  ")
      .append(brand_internal::padding)
      .append(brand_internal::border)
      .append(L"\x1b[38;2;255;215;0m")
      .append(bela::GetEnv(L"USERNAME"))
      .append(L"\x1b[0m@\x1b[38;2;0;191;255m")
      .append(bela::GetEnv(L"COMPUTERNAME"))
      .append(L"\x1b[0m\n");
  for (size_t i = 0; i < 6; i++) {
    text.append(L" ")
        .append(brand_internal::windows_colors[0])
        .append(brand_internal::padding)
        .append(L"\x1b[48;5;15m  ")
        .append(brand_internal::windows_colors[2])
        .append(brand_internal::padding)
        .append(L"\x1b[0m")
        .append(brand_internal::border);
    end_meta_line(text);
  }
  text.append(L" ")
      .append(L"\x1b[48;5;15m")
      .append(brand_internal::padding)
      .append(L"  ")
      .append(brand_internal::padding)
      .append(L"\x1b[0m")
      .append(brand_internal::border);
  end_meta_line(text);
  for (size_t i = 0; i < 6; i++) {
    text.append(L" ")
        .append(brand_internal::windows_colors[1])
        .append(brand_internal::padding)
        .append(L"\x1b[48;5;15m  ")
        .append(brand_internal::windows_colors[3])
        .append(brand_internal::padding)
        .append(L"\x1b[0m")
        .append(brand_internal::border);
    end_meta_line(text);
  }
  for (size_t i = 13; i < lines.size(); i++) {
    text.append(L" ")
        .append(brand_internal::padding)
        .append(L"  ")
        .append(brand_internal::padding)
        .append(brand_internal::border);
    end_meta_line(text);
  }
  return text;
}

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
  bool detect_storages(bela::error_code &ec);
  baulk::brand::processor processor;
  std::vector<monitor> monitors;
  std::vector<std::wstring> graphics;
  std::vector<storage> storages;
  memory_status mem_status;
};

void Detector::Swap(Render &render) const {
  using brand_internal::GiB;
  using brand_internal::MiB;
  auto version = bela::windows::version();
  render.version = version;
  auto is_windows_11 = version >= bela::windows::win11_21h2;
  auto edition = brand_internal::registry_string(HKEY_LOCAL_MACHINE, LR"(SOFTWARE\Microsoft\Windows NT\CurrentVersion)",
                                                 L"EditionID");

  // DisplayVersion
  // SOFTWARE\Microsoft\Windows NT\CurrentVersion\Update\TargetingInfo\Installed\Client.OS.rs2.amd64 version
  // SOFTWARE\Microsoft\Windows NT\CurrentVersion\Update\TargetingInfo\Installed\Windows.OOBE.amd64 version
  auto displayVersion = brand_internal::registry_string(
      HKEY_LOCAL_MACHINE, LR"(SOFTWARE\Microsoft\Windows NT\CurrentVersion)", L"DisplayVersion");
  render.append_meta_line(
      L"OS", bela::StringCat(L"Windows ", is_windows_11 ? L"11" : L"10", L" ", edition, L" ", displayVersion));
  render.append_meta_line(L"Kernel", bela::StringCat(version.major, L".", version.minor, L".", version.build));
  if (auto featurePack =
          brand_internal::registry_string(HKEY_LOCAL_MACHINE, brand_internal::update_stack_subkey, L"Version");
      !featurePack.empty()) {
    render.append_meta_line(L"FeatureExperiencePack", std::move(featurePack));
  }
  render.append_meta_line(L"Uptime", bela::FormatDuration(bela::Milliseconds(GetTickCount64())));
  render.append_meta_line(L"Terminal", brand_internal::TerminalName());

  // Resolution
  std::vector<std::wstring> resolution;
  for (const auto &m : monitors) {
    resolution.emplace_back(m());
  }
  render.append_meta_line(L"Resolution", bela::StrJoin(resolution, L", "));
  render.append_meta_line(L"WM", L"Desktop Window Manager");
  render.append_meta_line(L"CPU", processor());
  render.append_meta_line(L"GPU", bela::StrJoin(graphics, L", "));
  render.append_meta_line(L"RAM", bela::StringCat((mem_status.total - mem_status.avail) / MiB, L"MB / ",
                                                  mem_status.total / MiB, L"MB (", mem_status.load, L"%)"));
  std::vector<std::wstring> ss;
  for (const auto &so : storages) {
    ss.emplace_back(so());
  }
  render.append_meta_line(L"Disk", bela::StrJoin(ss, L", "));
  // COLOR
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
  detect_storages(ec);
  MEMORYSTATUSEX ms{0};
  ms.dwLength = sizeof(MEMORYSTATUSEX);
  GlobalMemoryStatusEx(&ms);
  mem_status.load = ms.dwMemoryLoad;
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
    auto close_adapter = bela::finally([&] { adapter->Release(); });
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
    graphics.emplace_back(desc1.Description);
  }
  return true;
}

inline bool Detector::detect_storages(bela::error_code &ec) {
  auto detect_drive = [&](uint8_t latter) {
    auto diskPath = bela::StringCat(static_cast<wchar_t>(latter), L":\\");
    switch (GetDriveType(diskPath.data())) {
    case DRIVE_REMOVABLE:
      break;
    case DRIVE_FIXED:
      break;
    case DRIVE_RAMDISK:
      break;
    default:
      return;
    }
    ULARGE_INTEGER freeBytesForCaller{.QuadPart = 0};
    ULARGE_INTEGER total{.QuadPart = 0};
    ULARGE_INTEGER avail{.QuadPart = 0};
    if (GetDiskFreeSpaceExW(diskPath.data(), &freeBytesForCaller, &total, &avail) != TRUE) {
      return;
    }
    storages.emplace_back(storage{
        .diskDevice = diskPath.substr(0, 2),
        .total = total.QuadPart,
        .avail = avail.QuadPart,
    });
  };

  auto N = GetLogicalDrives();
  for (size_t i = 0; i < sizeof(N) * 8; i++) {
    if (N == 0) {
      break;
    }
    if ((N & 1) != 0) {
      auto latter = 'A' + i;
      detect_drive(static_cast<uint8_t>(latter));
    }
    N >>= 1;
  }
  return true;
}

} // namespace baulk::brand

#endif
