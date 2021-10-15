//
#include <bela/path.hpp>
#include <bela/process.hpp>
#include <bela/simulator.hpp>
#include <bela/strip.hpp>
#include <baulk/fs.hpp>

namespace baulk::sevenzip {
constexpr auto ok = ERROR_SUCCESS;
inline std::optional<std::wstring> find7zInstallPath(bela::error_code &ec) {
  HKEY hkey = nullptr;
  if (RegOpenKeyW(HKEY_LOCAL_MACHINE, LR"(SOFTWARE\7-Zip-Zstandard)", &hkey) != ok) {
    if (RegOpenKeyW(HKEY_LOCAL_MACHINE, LR"(SOFTWARE\7-Zip)", &hkey) != ok) {
      ec = bela::make_system_error_code();
      return std::nullopt;
    }
  }
  auto closer = bela::finally([&] { RegCloseKey(hkey); });
  wchar_t buffer[4096];
  DWORD type = 0;
  DWORD bufsize = sizeof(buffer);
  if (RegQueryValueExW(hkey, L"Path64", nullptr, &type, reinterpret_cast<LPBYTE>(buffer), &bufsize) != ok &&
      type == REG_SZ) {
    if (auto s7z = bela::StringCat(bela::StripSuffix(buffer, L"\\"), L"\\7z.exe"); bela::PathExists(s7z)) {
      return std::make_optional(std::move(s7z));
    }
  }
  if (RegQueryValueExW(hkey, L"Path", nullptr, &type, reinterpret_cast<LPBYTE>(buffer), &bufsize) != ok) {
    ec = bela::make_system_error_code();
    return std::nullopt;
  }
  if (type != REG_SZ) {
    ec = bela::make_error_code(bela::ErrGeneral, L"reg key Path not REG_SZ: ", type);
    return std::nullopt;
  }
  if (auto s7z = bela::StringCat(bela::StripSuffix(buffer, L"\\"), L"\\7z.exe"); bela::PathExists(s7z)) {
    return std::make_optional(std::move(s7z));
  }
  ec = bela::make_error_code(ERROR_NOT_FOUND, L"7z.exe not found");
  return std::nullopt;
}

inline std::optional<std::wstring> lookup_sevenzip() {
  bela::error_code ec;
  auto parent = bela::ExecutableParent(ec);
  if (!parent) {
    return std::nullopt;
  }
  // baulk7z - A derivative version of 7-Zip-zstd maintained by baulk
  // contributors
  if (auto s7z = bela::StringCat(*parent, L"\\links\\baulk7z.exe"); bela::PathExists(s7z)) {
    return std::make_optional(std::move(s7z));
  }
  if (auto ps7z = find7zInstallPath(ec); ps7z) {
    return std::make_optional(std::move(*ps7z));
  }
  if (auto s7z = bela::StringCat(*parent, L"\\links\\7z.exe"); !bela::PathExists(s7z)) {
    return std::make_optional(std::move(s7z));
  }
  std::wstring s7z;
  if (bela::env::LookPath(L"7z.exe", s7z, true)) {
    return std::make_optional(std::move(s7z));
  }
  return std::nullopt;
}

bool Decompress(std::wstring_view src, std::wstring_view outdir, bela::error_code &ec) {
  auto s7z = lookup_sevenzip();
  if (!s7z) {
    ec = bela::make_error_code(ERROR_NOT_FOUND, L"7z not install");
    return false;
  }
  bela::process::Process process;
  if (process.Execute(*s7z, L"e", L"-spf", L"-y", src, bela::StringCat(L"-o", outdir)) != 0) {
    ec = process.ErrorCode();
    return false;
  }
  return true;
}
} // namespace baulk::sevenzip