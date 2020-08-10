//
#ifndef BAULK_BAULKENV_HPP
#define BAULK_BAULKENV_HPP
#include <bela/path.hpp>
#include <bela/base.hpp>
#include <bela/simulator.hpp>

namespace baulk::env {

#ifdef _M_X64
// Always build x64 binary
[[maybe_unused]] constexpr std::wstring_view HostArch = L"x64"; // Hostx64 x64
#else
[[maybe_unused]] constexpr std::wstring_view HostArch = L"x86"; // Hostx86 x86
#endif

struct BaulkVirtualEnv {
  std::vector<std::wstring> paths;
  std::vector<std::wstring> envs;
  std::vector<std::wstring> includes;
  std::vector<std::wstring> libs;
};

struct Searcher {
  Searcher(bela::env::Simulator &simulator_, std::wstring_view arch_ = HostArch)
      : simulator{simulator_}, arch(arch_) {
    if (arch.empty()) {
      arch = HostArch;
    }
  }
  using vector_t = std::vector<std::wstring>;
  bela::env::Simulator &simulator;
  std::wstring arch;
  std::wstring baulkroot;
  std::wstring baulkvfs;
  std::wstring baulketc;
  std::wstring baulkbindir;
  vector_t paths;
  vector_t libs;
  vector_t includes;
  vector_t libpaths;
  vector_t availableEnv;
  bool JoinEnvInternal(vector_t &vec, std::wstring &&p) {
    if (bela::PathExists(p)) {
      vec.emplace_back(std::move(p));
      return true;
    }
    return false;
  }
  void JoinForceEnv(vector_t &vec, std::wstring_view p) { vec.emplace_back(std::wstring(p)); }
  void JoinForceEnv(vector_t &vec, std::wstring_view a, std::wstring_view b) {
    vec.emplace_back(bela::StringCat(a, b));
  }
  bool JoinEnv(vector_t &vec, std::wstring_view p) { return JoinEnvInternal(vec, std::wstring(p)); }
  bool JoinEnv(vector_t &vec, std::wstring_view a, std::wstring_view b) {
    auto p = bela::StringCat(a, b);
    return JoinEnvInternal(vec, std::wstring(p));
  }
  bool JoinEnv(vector_t &vec, std::wstring_view a, std::wstring_view b, std::wstring_view c) {
    auto p = bela::StringCat(a, b, c);
    return JoinEnvInternal(vec, std::wstring(p));
  }
  bool JoinEnv(vector_t &vec, std::wstring_view a, std::wstring_view b, std::wstring_view c,
               std::wstring_view d) {
    auto p = bela::StringCat(a, b, c, d);
    return JoinEnvInternal(vec, std::wstring(p));
  }
  template <typename... Args>
  bool JoinEnv(vector_t &vec, std::wstring_view a, std::wstring_view b, std::wstring_view c,
               std::wstring_view d, Args... args) {
    auto p = bela::strings_internal::CatPieces({a, b, c, d, args...});
    return JoinEnvInternal(vec, std::wstring(p));
  }
  bool InitializeWindowsKitEnv(bela::error_code &ec);
  bool InitializeVisualStudioEnv(bool clang, bela::error_code &ec);
  bool InitializeBaulk(bela::error_code &ec);
  bool InitializeGit(bool cleanup, bela::error_code &ec);
  bool InitializeVirtualEnv(const std::vector<std::wstring> &venvs, bela::error_code &ec);
  // dot not call
  bool InitializeOneEnv(std::wstring_view pkgname, bela::error_code &ec);
  bool InitializeLocalEnv(std::wstring_view pkgname, BaulkVirtualEnv &venv, bela::error_code &ec);
  bool FlushEnv();
};

} // namespace baulk::env

#endif