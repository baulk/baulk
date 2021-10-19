// baulk virtual env
#include <bela/io.hpp>
#include <bela/process.hpp> // run vswhere
#include <bela/path.hpp>
#include <list>
#include <filesystem>
#include <baulk/registry.hpp>
#include <jsonex.hpp>
#include <baulkenv.hpp>

namespace baulk::env {
// Env Dependent Chain
class EnvDependentChain {
public:
  EnvDependentChain(Searcher &searcher_) : searcher(searcher_) {}
  EnvDependentChain(const EnvDependentChain &) = delete;
  EnvDependentChain &operator=(const EnvDependentChain &) = delete;
  bool InitializeEnvs(const std::vector<std::wstring> &venvs, bela::error_code &ec) {
    for (const auto &e : venvs) {
      if (!LoadEnv(e, ec)) {
        return false;
      }
    }
    return true;
  }
  bool FlushEnv();

private:
  bool PackageExists(std::wstring_view pkgname) {
    auto f = bela::StringCat(searcher.baulkbindir, L"\\locks\\", pkgname, L".json");
    return bela::PathFileIsExists(f);
  }
  bool LoadEnv(std::wstring_view pkgname, bela::error_code &ec);
  bool LoadLocalEnv(std::wstring_view pkgname, BaulkVirtualEnv &venv, bela::error_code &ec);
  bool LoadEnvImpl(std::wstring_view pkgname, BaulkVirtualEnv &venv, bela::error_code &ec);
  bool dependenciesExists(std::wstring_view name) {
    for (const auto &d : nodependsenv) {
      if (bela::EqualsIgnoreCase(d.name, name)) {
        return true;
      }
    }
    for (const auto &d : dependsenv) {
      if (bela::EqualsIgnoreCase(d.name, name)) {
        return true;
      }
    }
    return false;
  }
  Searcher &searcher;
  std::vector<baulk::env::BaulkVirtualEnv> nodependsenv;
  std::list<baulk::env::BaulkVirtualEnv> dependsenv;
  int depth{0};
};

void ReplaceDependencies(BaulkVirtualEnv &venv, const std::vector<std::wstring> &depends) {
  if (venv.dependencies.empty() || depends.empty()) {
    return;
  }
  auto replaceOne = [&](std::wstring &depend) {
    auto prefix = bela::StringCat(depend, L"=>");
    for (const auto &d : depends) {
      if (bela::StartsWithIgnoreCase(d, prefix)) {
        depend = d.substr(prefix.size());
        return;
      }
    }
  };
  for (auto &vd : venv.dependencies) {
    replaceOne(vd);
  }
}

bool EnvDependentChain::LoadEnv(std::wstring_view pkgname, bela::error_code &ec) {
  if (dependenciesExists(pkgname)) {
    return true;
  }
  BaulkVirtualEnv venv;
  venv.name = pkgname;
  if (!LoadEnvImpl(pkgname, venv, ec)) {
    return true;
  }
  if (venv.dependencies.empty()) {
    nodependsenv.emplace_back(std::move(venv));
    return true;
  }
  depth++;
  auto closer = [&] { depth--; };
  if (depth > 31) {
    // The dependency chain is too deep
    nodependsenv.emplace_back(std::move(venv));
    return true;
  }
  for (const auto &d : venv.dependencies) {
    if (!LoadEnv(d, ec)) {
      return false;
    }
  }
  dependsenv.push_back(std::move(venv));
  return true;
}

bool EnvDependentChain::LoadLocalEnv(std::wstring_view pkgname, BaulkVirtualEnv &venv, bela::error_code &ec) {
  auto localfile = bela::StringCat(searcher.baulkbindir, L"\\etc\\", pkgname, L".local.json");
  if (!bela::PathExists(localfile)) {
    return false;
  }
  FILE *fd = nullptr;
  if (auto en = _wfopen_s(&fd, localfile.data(), L"rb"); en != 0) {
    ec = bela::make_stdc_error_code(en, bela::StringCat(L"open ", pkgname, L".local.json' "));
    return false;
  }
  auto closer = bela::finally([&] { fclose(fd); });
  try {
    auto j = nlohmann::json::parse(fd, nullptr, true, true);
    baulk::json::JsonAssignor jea(j);
    jea.array("path", venv.paths);
    jea.array("env", venv.envs);
    jea.array("include", venv.includes);
    jea.array("lib", venv.libs);
    std::vector<std::wstring> replaceDependencies;
    jea.array("replace", replaceDependencies);
    ReplaceDependencies(venv, replaceDependencies);
  } catch (const std::exception &e) {
    ec = bela::make_error_code(bela::ErrGeneral, pkgname, L".local.json: ", bela::encode_into<char, wchar_t>(e.what()));
    return false;
  }
  return true;
}
// load env
bool EnvDependentChain::LoadEnvImpl(std::wstring_view pkgname, BaulkVirtualEnv &venv, bela::error_code &ec) {
  auto lockfile = bela::StringCat(searcher.baulkbindir, L"\\locks\\", pkgname, L".json");
  FILE *fd = nullptr;
  if (auto en = _wfopen_s(&fd, lockfile.data(), L"rb"); en != 0) {
    ec = bela::make_stdc_error_code(en, bela::StringCat(L"open 'locks\\", pkgname, L".json' "));
    searcher.DbgPrint(L"load '%s': %s", pkgname, ec.message);
    return false;
  }
  auto closer = bela::finally([&] { fclose(fd); });
  try {
    auto j = nlohmann::json::parse(fd, nullptr, true, true);
    if (auto it = j.find("venv"); it != j.end() && it.value().is_object()) {
      baulk::json::JsonAssignor jea(it.value());
      jea.array("path", venv.paths);
      jea.array("env", venv.envs);
      jea.array("include", venv.includes);
      jea.array("lib", venv.libs);
      jea.array("dependencies", venv.dependencies);
    }
  } catch (const std::exception &e) {
    ec = bela::make_error_code(bela::ErrGeneral, L"parse package ", pkgname, L" json: ", bela::encode_into<char, wchar_t>(e.what()));
    searcher.DbgPrint(L"decode '%s.json': %s", pkgname, ec.message);
    return false;
  }
  if (!LoadLocalEnv(pkgname, venv, ec)) {
    searcher.DbgPrint(L"%s no local setting", pkgname);
  }
  return true;
}

bool EnvDependentChain::FlushEnv() {
  auto envExists = [&](std::wstring_view e) {
    for (const auto ae : searcher.availableEnv) {
      if (bela::EqualsIgnoreCase(ae, e)) {
        return true;
      }
    }
    return false;
  };

  // support '~/'
  auto joinPathExpand = [&](const std::vector<std::wstring> &load, std::vector<std::wstring> &save,
                            bela::env::Simulator &sm) {
    for (const auto &x : load) {
      searcher.JoinForceEnv(save, sm.PathExpand(x));
    }
  };
  auto flushOnceEnv = [&](const BaulkVirtualEnv &e) {
    auto newSimulator = searcher.simulator;
    auto baulkpkgroot = bela::StringCat(searcher.baulkbindir, L"\\pkgs\\", e.name);
    newSimulator.SetEnv(L"BAULK_ROOT", searcher.baulkroot);
    newSimulator.SetEnv(L"BAULK_ETC", searcher.baulketc);
    newSimulator.SetEnv(L"BAULK_VFS", searcher.baulkvfs);
    newSimulator.SetEnv(L"BAULK_PKGROOT", baulkpkgroot);
    newSimulator.SetEnv(L"BAULK_BINDIR", searcher.baulkbindir);
    joinPathExpand(e.paths, searcher.paths, newSimulator);
    joinPathExpand(e.includes, searcher.includes, newSimulator);
    joinPathExpand(e.libs, searcher.libs, newSimulator);
    // set env k=v
    std::wstring buffer;
    // ENV not support ~/
    for (const auto &e : e.envs) {
      buffer.clear();
      newSimulator.ExpandEnv(e, buffer);
      searcher.simulator.PutEnv(buffer, true);
    }
    searcher.availableEnv.emplace_back(e.name);
  };
  for (const auto e : nodependsenv) {
    if (envExists(e.name)) {
      searcher.DbgPrint(L"venv: %s has been loaded", e.name);
      continue;
    }
    searcher.DbgPrint(L"venv: %s no dependencies", e.name);
    flushOnceEnv(e);
  }
  for (const auto e : dependsenv) {
    if (envExists(e.name)) {
      searcher.DbgPrint(L"venv: %s has been loaded", e.name);
      continue;
    }
    searcher.DbgPrint(L"venv: %s depend on: %s", e.name, bela::StrJoin(e.dependencies, L", "));
    flushOnceEnv(e);
  }
  return true;
}

// baulk virtual env
bool Searcher::InitializeVirtualEnv(const std::vector<std::wstring> &venvs, bela::error_code &ec) {
  EnvDependentChain chain(*this);
  if (!chain.InitializeEnvs(venvs, ec)) {
    return false;
  }
  chain.FlushEnv();
  return true;
}

} // namespace baulk::env