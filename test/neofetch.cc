//
#include <bela/base.hpp>
#include <bela/terminal.hpp>
#include <dxgi.h>

#if defined(_M_X64) || defined(_M_X86)
#include <intrin.h>
inline std::wstring resolve_cpu_brand() {
  int cpuinfo[4];
  __cpuidex(cpuinfo, 0x4, 0);
  auto ncores = (cpuinfo[0] >> 26) + 1;
  bela::FPrintF(stderr, L"cores: %d\n", ncores);
  int cpubrand[4 * 3];
  __cpuid(&cpubrand[0], 0x80000002);
  __cpuid(&cpubrand[4], 0x80000003);
  __cpuid(&cpubrand[8], 0x80000004);
  auto cpu_brand = reinterpret_cast<const char *>(cpubrand);
  return bela::encode_into<char, wchar_t>(
      {reinterpret_cast<const char *>(cpubrand), strnlen(cpu_brand, sizeof(cpubrand))});
}

#else
inline std::wstring resolve_cpu_brand() { return L"ARM64"; }
#endif

/*

           .-/+oossssoo+/-.               sk@ostechnix
        `:+ssssssssssssssssss+:`           ------------
      -+ssssssssssssssssssyyssss+-         OS: Ubuntu 20.04 LTS x86_64
    .ossssssssssssssssssdMMMNysssso.       Host: Inspiron N5050
   /ssssssssssshdmmNNmmyNMMMMhssssss/      Kernel: 5.4.0-37-generic
  +ssssssssshmydMMMMMMMNddddyssssssss+     Uptime: 5 hours, 46 mins
 /sssssssshNMMMyhhyyyyhmNMMMNhssssssss/    Packages: 2378 (dpkg), 7 (flatpak), 11 (snap)
.ssssssssdMMMNhsssssssssshNMMMdssssssss.   Shell: bash 5.0.16
+sssshhhyNMMNyssssssssssssyNMMMysssssss+   Resolution: 1366x768
ossyNMMMNyMMhsssssssssssssshmmmhssssssso   DE: GNOME
ossyNMMMNyMMhsssssssssssssshmmmhssssssso   WM: Mutter
+sssshhhyNMMNyssssssssssssyNMMMysssssss+   WM Theme: Adwaita
.ssssssssdMMMNhsssssssssshNMMMdssssssss.   Theme: Yaru-light [GTK2/3]
 /sssssssshNMMMyhhyyyyhdNMMMNhssssssss/    Icons: ubuntu-mono-light [GTK2/3]
  +sssssssssdmydMMMMMMMMddddyssssssss+     Terminal: deepin-terminal
   /ssssssssssshdmNNNNmyNMMMMhssssss/      Terminal Font: Ubuntu Mono 12
    .ossssssssssssssssssdMMMNysssso.       CPU: Intel i3-2350M (4) @ 2.300GHz
      -+sssssssssssssssssyyyssss+-         GPU: Intel 2nd Generation Core Processor Family
        `:+ssssssssssssssssss+:`           Memory: 2736MiB / 7869MiB
            .-/+oossssoo+/-.

*/

constexpr std::wstring_view space = L"                    ";

constexpr std::wstring_view windows10 = LR"(                    ....,,:;+ccllll
      ...,,+:;  cllllllllllllllllll
,cclllllllllll  lllllllllllllllllll
llllllllllllll  lllllllllllllllllll
llllllllllllll  lllllllllllllllllll
llllllllllllll  lllllllllllllllllll
llllllllllllll  lllllllllllllllllll
llllllllllllll  lllllllllllllllllll
llllllllllllll  lllllllllllllllllll
llllllllllllll  lllllllllllllllllll
llllllllllllll  lllllllllllllllllll
llllllllllllll  lllllllllllllllllll
llllllllllllll  lllllllllllllllllll
`'ccllllllllll  lllllllllllllllllll
       `' \\*::  :ccllllllllllllllll
                       ````''*::cll
                                 ``)";

void find_monitors() {
  auto n = GetSystemMetrics(SM_CMONITORS);
  bela::FPrintF(stderr, L"Find phy monitors: %d\n", n);
  for (int i = 0; i < n; i++) {
    DISPLAY_DEVICEW device{
        .cb = sizeof(DISPLAY_DEVICEW),
        .DeviceName = {0},
        .DeviceString = {0},
        .StateFlags = 0,
        .DeviceID = {0},
        .DeviceKey = {0},
    };
    if (EnumDisplayDevicesW(NULL, i, &device, 0) != TRUE) {
      auto ec = bela::make_system_error_code();
      bela::FPrintF(stderr, L"EnumDisplayDevicesW error: %s\n", ec);
      continue;
    }
    DEVMODEW device_mode;
    device_mode.dmSize = sizeof(device_mode);
    device_mode.dmDriverExtra = 0;
    if (EnumDisplaySettingsExW(device.DeviceName, ENUM_CURRENT_SETTINGS, &device_mode, 0) != TRUE) {
      auto ec = bela::make_system_error_code();
      bela::FPrintF(stderr, L"EnumDisplayDevicesW error: %s\n", ec);
      continue;
    }
    bela::FPrintF(stderr, L"\x1b[31mResolution\x1b[0m: %dx%d %dHz <LogPixels: %d>\n", device_mode.dmPelsWidth,
                  device_mode.dmPelsHeight, device_mode.dmDisplayFrequency, device_mode.dmLogPixels);
  }
}

// https://docs.microsoft.com/zh-cn/windows/win32/direct3ddxgi/d3d10-graphics-programming-guide-dxgi?redirectedfrom=MSDN#WARP_new_for_Win8
void find_graphics_device() {
  IDXGIFactory1 *factory{nullptr};
  IDXGIAdapter1 *adapter{nullptr};
  auto closer = bela::finally([&] {
    if (factory != nullptr) {
      factory->Release();
    }
  });
  if (auto hr = CreateDXGIFactory1(__uuidof(IDXGIFactory1), reinterpret_cast<void **>(&factory)); FAILED(hr)) {
    auto ec = bela::make_system_error_code();
    bela::FPrintF(stderr, L"CreateDXGIFactory1 error: %s\n", ec);
    return;
  }
  UINT Adapter = 0;
  while (factory->EnumAdapters1(Adapter, &adapter) != DXGI_ERROR_NOT_FOUND) {
    Adapter++;
    DXGI_ADAPTER_DESC1 desc1;
    if (adapter->GetDesc1(&desc1) != S_OK) {
      auto ec = bela::make_system_error_code();
      bela::FPrintF(stderr, L"GetDesc1 error: %s\n", ec);
      continue;
    }
    if ((desc1.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) != 0) {
      // Microsoft Basic Render Driver
      continue;
    }
    bela::FPrintF(stderr, L"GPU: %s VendorId: %d\n", desc1.Description, desc1.VendorId);
    UINT displayNum = 0;
    IDXGIOutput *out = nullptr;
    while (adapter->EnumOutputs(displayNum, &out) != DXGI_ERROR_NOT_FOUND) {
      displayNum++;
      DXGI_OUTPUT_DESC desc;
      if (out->GetDesc(&desc) != S_OK) {
        continue;
      }
      bela::FPrintF(stderr, L"Find display device: %s %dx%d\n", desc.DeviceName,
                    desc.DesktopCoordinates.right - desc.DesktopCoordinates.left,
                    desc.DesktopCoordinates.bottom - desc.DesktopCoordinates.top);
    }
  }
}

void find_processor() {
  bela::FPrintF(stderr, L"CPU: %s\n", resolve_cpu_brand());
  SYSTEM_INFO si;
  GetSystemInfo(&si);
  bela::FPrintF(stderr, L"dwNumberOfProcessors %d\n", si.dwNumberOfProcessors);
}

void find_processor2() {

  char answer[64] = "Error Reading CPU Name from Registry!", inBuffer[64] = "";
  const char *csName = "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0";
  HKEY hKey;
  DWORD gotType, gotSize = BUFSIZ;
  if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, csName, 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
    if (!RegQueryValueExA(hKey, "ProcessorNameString", nullptr, &gotType, (PBYTE)(inBuffer), &gotSize)) {
      if ((gotType == REG_SZ) && strlen(inBuffer))
        strcpy(answer, inBuffer);
    }
    RegCloseKey(hKey);
  }
  bela::FPrintF(stderr, L"CPU: %s\n", answer);
}

// 10,132,217
// 243,243,243
int wmain() {
  bela::FPrintF(stderr, L"Windows 11 21h1\n");
  for (int i = 0; i < 9; i++) {
    bela::FPrintF(stderr, L"\x1b[48;5;39m%s\x1b[48;5;15m  \x1b[48;5;39m%s\x1b[0m\n", space, space);
  }
  bela::FPrintF(stderr, L"\x1b[48;5;15m%s  %s\x1b[0m\n", space, space);
  for (int i = 0; i < 9; i++) {
    bela::FPrintF(stderr, L"\x1b[48;5;39m%s\x1b[48;5;15m  \x1b[48;5;39m%s\x1b[0m\n", space, space);
  }
  bela::FPrintF(stderr, L"Windows 10\n\x1b[38;5;39m%s\x1b[0m\n", windows10);
  find_monitors();
  find_graphics_device();
  find_processor();
  find_processor2();
  //   DWORD dwSize = 0;
  //   GetLogicalProcessorInformationEx(RelationAll, nullptr, &dwSize);
  //   std::vector<char> data;
  //   data.resize(dwSize);
  //   auto N = dwSize / sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX);
  //   auto p = reinterpret_cast<PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX>(data.data());
  //   GetLogicalProcessorInformationEx(RelationAll, p, &dwSize);
  //   for (int i = 0; i < N; i++) {
  //     bela::FPrintF(stderr, L"Logical core: %d %d\n", i, static_cast<int>(p[i].Size));
  //   }
  return 0;
}