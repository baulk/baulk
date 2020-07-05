// baulk virtual env
#include <bela/io.hpp>
#include <bela/process.hpp> // run vswhere
#include <bela/path.hpp>
#include <bela/env.hpp>
#include <filesystem>
#include <regutils.hpp>
#include <jsonex.hpp>
#include <baulkenv.hpp>

namespace baulk::env {
// baulk virtual env
bool Searcher::InitializeVirtualEnv(const std::vector<std::wstring> &venvs, bela::error_code &ec) {
  for (const auto &p : venvs) {
    InitializeOneEnv(p, ec);
  }
  return true;
}

struct BaulkVirtualEnv {
  std::vector<std::wstring> paths;
  std::vector<std::wstring> envs;
};

bool Searcher::InitializeOneEnv(std::wstring_view pkgname, bela::error_code &ec) {
  auto lockfile = bela::StringCat(baulkbindir, L"\\locks\\", pkgname, L".json");
  FILE *fd = nullptr;
  if (auto en = _wfopen_s(&fd, lockfile.data(), L"rb"); en != 0) {
    ec = bela::make_stdc_error_code(en, bela::StringCat(L"open 'locks\\", pkgname, L".json' "));
    return false;
  }
  auto closer = bela::finally([&] { fclose(fd); });
  BaulkVirtualEnv venv;
  try {
    auto j = nlohmann::json::parse(fd);
    if (auto it = j.find("venv"); it != j.end() && it.value().is_object()) {
      baulk::json::JsonAssignor jea(it.value());
      jea.array("path", venv.paths);
      jea.array("env", venv.envs);
    }
  } catch (const std::exception &e) {
    ec = bela::make_error_code(1, L"parse package ", pkgname, L" json: ", bela::ToWide(e.what()));
    return false;
  }
  bela::env::Derivator xdev;
  xdev.SetEnv(L"PKGROOT", bela::StringCat(baulkbindir, L"\\pkgs\\", pkgname));
  xdev.SetEnv(L"BAULK_BINDIR", baulkbindir);
  for (const auto &p : venv.paths) {
    std::wstring path_;
    xdev.ExpandEnv(p, path_);
    JoinEnv(paths, path_);
  }
  for (const auto &e : venv.envs) {
    std::wstring value;
    xdev.ExpandEnv(e, value);
    dev.PutEnv(value);
  }
  return true;
}

} // namespace baulk::env