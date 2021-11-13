//
#include <bela/base.hpp>
#include <bela/terminal.hpp>
#include <dxgi.h>
#include <baulk/brand.hpp>

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
  baulk::brand::Detector detector;
  bela::error_code ec;
  if (!detector.Execute(ec)) {
    bela::FPrintF(stderr, L"detect: %s\n", ec);
    return 1;
  }
  baulk::brand::Render render;
  detector.Swap(render);
  auto buffer = render.DrawWindows10();
  bela::FPrintF(stderr, L"%s\n", buffer);
  auto buffer2 = render.DrawWindows11();
  bela::FPrintF(stderr, L"\n%s\n", buffer2);
  return 0;
}