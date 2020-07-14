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

bool Searcher::InitializeLocalEnv(std::wstring_view pkgname, BaulkVirtualEnv &venv,
                                  bela::error_code &ec) {
  auto lockfile = bela::StringCat(baulkbindir, L"\\etc\\", pkgname, L".local.json");
  FILE *fd = nullptr;
  if (auto en = _wfopen_s(&fd, lockfile.data(), L"rb"); en != 0) {
    ec = bela::make_stdc_error_code(en, bela::StringCat(L"open 'locks\\", pkgname, L".json' "));
    return false;
  }
  auto closer = bela::finally([&] { fclose(fd); });
  try {
    auto j = nlohmann::json::parse(fd);
    baulk::json::JsonAssignor jea(j);
    jea.array("path", venv.paths);
    jea.array("env", venv.envs);
    jea.array("include", venv.includes);
    jea.array("lib", venv.libs);
  } catch (const std::exception &e) {
    ec = bela::make_error_code(1, L"parse package ", pkgname, L" json: ", bela::ToWide(e.what()));
    return false;
  }
  return true;
}

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
      jea.array("include", venv.includes);
      jea.array("lib", venv.libs);
    }
  } catch (const std::exception &e) {
    ec = bela::make_error_code(1, L"parse package ", pkgname, L" json: ", bela::ToWide(e.what()));
    return false;
  }
  InitializeLocalEnv(pkgname, venv, ec);
  bela::env::Derivator expanddev;
  auto baulkpkgroot = bela::StringCat(baulkbindir, L"\\pkgs\\", pkgname);
  expanddev.SetEnv(L"BAULK_PKGROOT", baulkpkgroot);
  expanddev.SetEnv(L"BAULK_BINDIR", baulkbindir);
  expanddev.SetEnv(L"BAULK_ROOT", baulkroot);
  std::wstring buffer;
  auto joinExpandEnv = [&](const vector_t &load, vector_t &save) {
    for (const auto &x : load) {
      buffer.clear();
      expanddev.ExpandEnv(x, buffer);
      JoinEnv(save, buffer);
    }
  };
  joinExpandEnv(venv.paths, paths);
  joinExpandEnv(venv.includes, includes);
  joinExpandEnv(venv.libs, libs);
  // set env k=v
  for (const auto &e : venv.envs) {
    buffer.clear();
    expanddev.ExpandEnv(e, buffer);
    dev.PutEnv(buffer, true);
  }
  return true;
}

} // namespace baulk::env