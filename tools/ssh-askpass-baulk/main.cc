/// ssh-askpass-baulk
#include <Windows.h>
#include <wincred.h>
#include <string_view>
#include <bela/codecvt.hpp> // test prompt length
#include <bela/str_split.hpp>
#include <bela/parseargv.hpp>
#include <bela/fmt.hpp>
#include <bela/terminal.hpp>
#include "version.h"
#include "app.hpp"

int credentials(std::wstring_view prompt, std::wstring_view user) {
  baulk::App app(prompt, L"Askpass Utility for Baulk", false);
  // baulk::App app(L"Enter your SSH Key Passphrase", L"Askpass Utility for
  // Baulk", false);
  return app.run(GetModuleHandle(nullptr));
}

void usage() {
  constexpr std::wstring_view usage =
      LR"(ssh-askpass-baulk - Askpass Utility for Baulk
Usage: ssh-askpass-baulk [option] ...
  -h|--help        Show usage text and quit
  -v|--version     Show version number and quit
  -V|--verbose     Make the operation more talkative
  -p|--password    Turn on password mode.

)";
  bela::terminal::WriteAuto(stderr, usage);
}

bool ParseArgv(int argc, wchar_t **argv, std::wstring &prompt, bool &credmode) {
  bela::ParseArgv pa(argc, argv);
  pa.Add(L"help", bela::no_argument, 'h')
      .Add(L"version", bela::no_argument, L'v')
      .Add(L"password", bela::no_argument, 'p')
      .Add(L"user", bela::no_argument, 'u');
  bela::error_code ec;
  auto ret = pa.Execute(
      [&](int val, const wchar_t *oa, const wchar_t *) {
        switch (val) {
        case 'h':
          usage();
          exit(0);
        case 'v':
          bela::FPrintF(stdout, L"ssh-askpass-baulk version: %s\n",
                        BAULK_VERSION_FULL);
          exit(0);
        case 'p':
          credmode = true;
          break;
        case 'u':
          break;
        default:
          return false;
        }
        return true;
      },
      ec);
  if (!ret) {
    bela::FPrintF(stderr, L"ParseArgv: %s\n", ec.message);
    exit(0);
  }
  if (pa.UnresolvedArgs().empty()) {
    prompt = L"Enter your SSH Key Passphrase";
    return true;
  }
  for (const auto s : pa.UnresolvedArgs()) {
    if (!prompt.empty()) {
      prompt.append(L"\n");
    }
    prompt.append(s);
  }
  return true;
}

int wmain(int argc, wchar_t **argv) {
  std::wstring prompt;
  bool credmode = false;
  ParseArgv(argc, argv, prompt, credmode);
  baulk::App app(prompt, L"Askpass Utility for Baulk", !credmode);
  return app.run(GetModuleHandle(nullptr));
}