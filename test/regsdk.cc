//
#include <bela/terminal.hpp>
#include <bela/io.hpp>
#include <bela/match.hpp>
#include <filesystem>
#include <baulk/registry.hpp>
#include <jsonex.hpp>

bool SDKSearchVersion(std::wstring_view sdkroot, std::wstring_view sdkver, std::wstring &sdkversion) {
  auto dir = bela::StringCat(sdkroot, L"\\Include");
  for (auto &p : std::filesystem::directory_iterator(dir)) {
    bela::FPrintF(stderr, L"Lookup: %s\n", p.path().wstring());
    auto filename = p.path().filename().wstring();
    if (bela::StartsWith(filename, sdkver)) {
      sdkversion = filename;
      return true;
    }
  }
  return false;
}

std::optional<std::wstring> LookupVisualCppVersion(std::wstring_view vsdir, bela::error_code &ec) {
  auto file = bela::StringCat(vsdir, L"/VC/Auxiliary/Build/Microsoft.VCToolsVersion.default.txt");
  auto line = bela::io::ReadLine(file, ec);
  if (!line) {
    return std::nullopt;
  }
  return std::make_optional(std::move(*line));
}

int wmain(int argc, wchar_t **argv) {
  //
  bela::error_code ec;
  auto winsdk = baulk::registry::LookupWindowsSDK(ec);
  if (!winsdk) {
    bela::FPrintF(stderr, L"unable to find windows sdk %s\n", ec.message);
    return 1;
  }
  std::wstring sdkversion;
  if (!SDKSearchVersion(winsdk->InstallationFolder, winsdk->ProductVersion, sdkversion)) {
    bela::FPrintF(stderr, L"invalid sdk version");
    return 1;
  }

  bela::FPrintF(stderr,
                L"InstallationFolder: '%s'\nProductVersion: '%s'\nSDKVersion: "
                L"'%s'\n",
                winsdk->InstallationFolder, winsdk->ProductVersion, sdkversion);

  auto vcversion = LookupVisualCppVersion(LR"(C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\)", ec);
  if (!vcversion) {
    bela::FPrintF(stderr, L"unable parse: %s\n", ec.message);
    return 1;
  }
  bela::FPrintF(stderr, L"Visual C++: '%s'\n", *vcversion);
  return 0;
}