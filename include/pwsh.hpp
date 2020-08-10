//
#ifndef BAULK_PWSH_HPP
#define BAULK_PWSH_HPP
#include <bela/base.hpp>
#include <bela/env.hpp>
#include <bela/path.hpp>
#include <bela/numbers.hpp>
#include <filesystem>

namespace baulk::pwsh {
enum struct prerelease : std::uint8_t {
  alpha = 0, // alpha
  beta = 1,
  rc = 2,
  none = 3
};
struct PwshMeta {
  std::wstring path;
  int version{0};
  prerelease preview{prerelease::none};
  [[nodiscard]] constexpr int compare(const PwshMeta &other) const noexcept {
    if (version != other.version) {
      return version - other.version;
    }
    if (preview != other.preview) {
      return static_cast<std::uint8_t>(preview) - static_cast<std::uint8_t>(other.preview);
    }
    return 0;
  }
};

std::wstring PwshNewest(const std::vector<PwshMeta> &pwshs) {
  if (pwshs.empty()) {
    return L"";
  }
  size_t pos = 0;
  for (size_t i = 1; i < pwshs.size(); i++) {
    if (pwshs[i].compare(pwshs[pos]) > 0) {
      pos = i;
    }
  }
  return pwshs[pos].path;
}

std::wstring PwshPreview(const std::vector<PwshMeta> &pwshs) {
  for (auto &p : pwshs) {
    if (p.preview == prerelease::rc) {
      return p.path;
    }
  }
  return L"";
}

inline bool AccumulatePwsh(std::wstring_view dir, std::vector<PwshMeta> &pwshs) {
  auto pwshdir = bela::WindowsExpandEnv(dir);
  if (!bela::PathExists(pwshdir)) {
    return false;
  }
  constexpr const std::wstring_view pwsh_exe = L"pwsh.exe";
  constexpr const std::wstring_view previewsv = L"-preview";
  for (const auto &it : std::filesystem::directory_iterator(pwshdir)) {
    const auto versionPath = it.path();
    const auto exe = versionPath / pwsh_exe;
    if (std::filesystem::exists(exe)) {
      int version = 0;
      prerelease preview{prerelease::none};
      auto vstr = versionPath.filename().wstring();
      std::wstring_view sv(vstr);
      if (bela::EndsWithIgnoreCase(sv, previewsv)) {
        sv.remove_suffix(previewsv.size());
        preview = prerelease::rc;
      }
      bela::SimpleAtoi(sv, &version);
      pwshs.emplace_back(PwshMeta{exe.wstring(), version, preview});
    }
  }
  return !pwshs.empty();
}

inline std::wstring PwshCore() {
  std::vector<PwshMeta> pwshs;
  if (AccumulatePwsh(L"%ProgramFiles%\\PowerShell", pwshs)) {
    return PwshNewest(pwshs);
  }
#if defined(_M_AMD64) || defined(_M_ARM64)
  // No point in looking for WOW if we're not somewhere it exists
  if (AccumulatePwsh(L"%ProgramFiles(x86)%\\PowerShell", pwshs)) {
    return PwshNewest(pwshs);
  }
#endif
#if defined(_M_ARM64)
  // no point in looking for WOA if we're not on ARM64
  if (AccumulatePwsh(L"%ProgramFiles(Arm)%\\PowerShell", pwshs)) {
    return PwshNewest(pwshs);
  }
#endif
  return L"";
}

inline std::wstring PwshCorePreview() {
  std::vector<PwshMeta> pwshs;
  if (AccumulatePwsh(L"%ProgramFiles%\\PowerShell", pwshs)) {
    return PwshPreview(pwshs);
  }
#if defined(_M_AMD64) || defined(_M_ARM64)
  // No point in looking for WOW if we're not somewhere it exists
  if (AccumulatePwsh(L"%ProgramFiles(x86)%\\PowerShell", pwshs)) {
    return PwshPreview(pwshs);
  }
#endif
#if defined(_M_ARM64)
  // no point in looking for WOA if we're not on ARM64
  if (AccumulatePwsh(L"%ProgramFiles(Arm)%\\PowerShell", pwshs)) {
    return PwshPreview(pwshs);
  }
#endif
  return L"";
}

inline std::wstring PwshExePath() {
  if (auto pwsh = PwshCore(); !pwsh.empty()) {
    return pwsh;
  }
  if (auto pwsh = bela::WindowsExpandEnv(
          L"%SystemRoot%\\System32\\WindowsPowerShell\\v1.0\\powershell.exe");
      bela::PathExists(pwsh)) {
    return pwsh;
  }
  return L"";
}

} // namespace baulk::pwsh

#endif