//
#ifndef BAULK_VENV_HPP
#define BAULK_VENV_HPP
#include <bela/terminal.hpp>
#include <bela/env.hpp>
#include <bela/simulator.hpp>
#include "json_utils.hpp"
#include "vfs.hpp"

namespace baulk::env {
struct PackageEnv {
  std::wstring name;
  std::vector<std::wstring> paths;
  std::vector<std::wstring> envs;
  std::vector<std::wstring> includes;
  std::vector<std::wstring> libs;
  std::vector<std::wstring> dependencies;
};

inline void ReplaceDependencies(PackageEnv &pkgEnv, const std::vector<std::wstring> &depends) {
  if (pkgEnv.dependencies.empty() || depends.empty()) {
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
  for (auto &vd : pkgEnv.dependencies) {
    replaceOne(vd);
  }
}

class Constructor {
public:
  using vector_t = std::vector<std::wstring>;
  Constructor(bool isDebugMode = false) : IsDebugMode{isDebugMode} {}
  Constructor(const Constructor &) = delete;
  Constructor &operator=(const Constructor &) = delete;
  bool InitializeEnvs(const std::vector<std::wstring> &envs, bela::env::Simulator &sm, bela::error_code &ec) {
    if (envs.empty()) {
      return true;
    }
    for (auto &e : envs) {
      if (!loadOneEnv(bela::AsciiStrToLower(e), ec)) {
        return false;
      }
    }
    return flushEnv(sm, ec);
  }

private:
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
    return JoinEnvInternal(vec, bela::StringCat(a, b));
  }
  bool JoinEnv(vector_t &vec, std::wstring_view a, std::wstring_view b, std::wstring_view c) {
    return JoinEnvInternal(vec, bela::StringCat(a, b, c));
  }
  bool JoinEnv(vector_t &vec, std::wstring_view a, std::wstring_view b, std::wstring_view c, std::wstring_view d) {
    return JoinEnvInternal(vec, bela::StringCat(a, b, c, d));
  }
  template <typename... Args>
  bool JoinEnv(vector_t &vec, std::wstring_view a, std::wstring_view b, std::wstring_view c, std::wstring_view d,
               Args... args) {
    return JoinEnvInternal(vec, bela::strings_internal::CatPieces({a, b, c, d, args...}));
  }
  void simulatorSetEnv(bela::env::Simulator &sm, std::wstring_view key, std::wstring_view value) {
    auto oldValue = sm.GetEnv(key);
    if (oldValue.empty()) {
      sm.SetEnv(key, value);
      return;
    }
    sm.SetEnv(key, bela::StringCat(value, L";", oldValue), true);
  }

  bool flushEnv(bela::env::Simulator &sm, bela::error_code &ec) {
    std::vector<std::wstring> availableEnv;
    auto envExists = [&](std::wstring_view e) {
      for (const auto ae : availableEnv) {
        if (bela::EqualsIgnoreCase(ae, e)) {
          return true;
        }
      }
      return false;
    };
    bela::env::Simulator cleanedSimulator;
    cleanedSimulator.InitializeCleanupEnv();
    std::vector<std::wstring> paths;
    std::vector<std::wstring> envs;
    std::vector<std::wstring> includes;
    std::vector<std::wstring> libs;
    // support '~/'
    auto joinPathExpand = [&](const std::vector<std::wstring> &load, std::vector<std::wstring> &save,
                              bela::env::Simulator &sm) {
      for (const auto &x : load) {
        JoinForceEnv(save, sm.PathExpand(x));
      }
    };
    auto flushOnceEnv = [&](const PackageEnv &e) {
      auto newSimulator = cleanedSimulator;
      auto pkgRoot = baulk::vfs::AppPackageRoot(e.name);
      newSimulator.SetEnv(L"BAULK_ROOT", baulk::vfs::AppBasePath());
      newSimulator.SetEnv(L"BAULK_VFS", baulk::vfs::AppUserVFS());
      newSimulator.SetEnv(L"BAULK_USER_VFS", baulk::vfs::AppUserVFS());
      newSimulator.SetEnv(L"BAULK_PKGROOT", pkgRoot);
      newSimulator.SetEnv(L"BAULK_PACKAGE_ROOT", pkgRoot);
      joinPathExpand(e.paths, paths, newSimulator);
      joinPathExpand(e.includes, includes, newSimulator);
      joinPathExpand(e.libs, libs, newSimulator);
      // set env k=v
      std::wstring buffer;
      // ENV not support ~/
      for (const auto &e : e.envs) {
        buffer.clear();
        newSimulator.ExpandEnv(e, buffer);
        sm.PutEnv(buffer, true);
      }
      availableEnv.emplace_back(e.name);
    };
    if (!libs.empty()) {
      simulatorSetEnv(sm, L"LIB", bela::JoinEnv(libs));
    }
    if (!includes.empty()) {
      simulatorSetEnv(sm, L"INCLUDE", bela::JoinEnv(includes));
    }
    for (const auto e : standardEnvs) {
      if (envExists(e.name)) {
        DbgPrint(L"venv: %s has been loaded", e.name);
        continue;
      }
      flushOnceEnv(e);
      DbgPrint(L"venv: %s no dependencies", e.name);
    }
    for (const auto e : requiresEnvs) {
      if (envExists(e.name)) {
        DbgPrint(L"venv: %s has been loaded", e.name);
        continue;
      }
      DbgPrint(L"venv: %s depend on: %s", e.name, bela::StrJoin(e.dependencies, L", "));
      flushOnceEnv(e);
    }
    sm.PathPushFront(std::move(paths));
    return true;
  }
  std::optional<PackageEnv> loadPackageEnv(std::wstring_view pkgName, bela::error_code &ec) {
    auto jo = baulk::json::parse_file(bela::StringCat(baulk::vfs::AppLocks(), L"\\", pkgName, L".json"), ec);
    if (!jo) {
      return std::nullopt;
    }
    PackageEnv pkgEnv{.name = std::wstring(pkgName)};
    auto jv = jo->view();
    if (auto sv = jv.subview("venv"); sv) {
      sv->fetch_paths_checked("path", pkgEnv.paths);
      sv->fetch_paths_checked("include", pkgEnv.includes);
      sv->fetch_paths_checked("lib", pkgEnv.libs);
      sv->fetch_strings_checked("env", pkgEnv.envs);
      sv->fetch_strings_checked("dependencies", pkgEnv.dependencies);
    }
    if (loadPackageLocalEnv(pkgName, pkgEnv, ec)) {
      // TODO
      DbgPrint(L"venv: %s found local env", pkgName);
    }
    return std::make_optional(std::move(pkgEnv));
  }
  bool loadPackageLocalEnv(std::wstring_view pkgName, PackageEnv &pkgEnv, bela::error_code &ec) {
    auto jo = baulk::json::parse_file(bela::StringCat(baulk::vfs::AppEtc(), L"\\", pkgName, L".local.json"), ec);
    if (!jo) {
      ec.clear();
      return true;
    }
    auto jv = jo->view();
    jv.fetch_paths_checked("path", pkgEnv.paths);
    jv.fetch_paths_checked("include", pkgEnv.includes);
    jv.fetch_paths_checked("lib", pkgEnv.libs);
    jv.fetch_strings_checked("env", pkgEnv.envs);
    std::vector<std::wstring> replaceDependencies;
    // local config can replace config
    if (jv.fetch_strings_checked("replace", replaceDependencies)) {
      ReplaceDependencies(pkgEnv, replaceDependencies);
      DbgPrint(L"venv: %s found replace --> ['%s']", pkgName, bela::StrJoin(replaceDependencies, L"', '"));
    }
    return true;
  }
  // LoadOneEnv
  bool loadOneEnv(std::wstring_view env, bela::error_code &ec) {
    if (envLoaded(env)) {
      return true;
    }
    auto pkgEnv = loadPackageEnv(env, ec);
    if (!pkgEnv) {
      return depth <= 1;
    }
    if (pkgEnv->dependencies.empty()) {
      standardEnvs.emplace_back(std::move(*pkgEnv));
      return true;
    }
    depth++;
    auto closer = [&] { depth--; };
    if (depth > 31) {
      // The dependency chain is too deep
      standardEnvs.emplace_back(std::move(*pkgEnv));
      return true;
    }
    for (const auto &d : pkgEnv->dependencies) {
      if (!loadOneEnv(d, ec)) {
        return false;
      }
    }
    requiresEnvs.push_back(std::move(*pkgEnv));
    return true;
  }
  bool envLoaded(const std::wstring_view env) const {
    for (const auto &se : standardEnvs) {
      if (se.name == env) {
        return true;
      }
    }
    for (auto &re : requiresEnvs) {
      if (re.name == env) {
        return true;
      }
    }
    return false;
  }
  template <typename... Args> bela::ssize_t DbgPrint(const wchar_t *fmt, const Args &...args) {
    if (!IsDebugMode) {
      return 0;
    }
    const bela::format_internal::FormatArg arg_array[] = {args...};
    std::wstring str;
    str.append(L"\x1b[33m* ");
    bela::format_internal::StrAppendFormatInternal(&str, fmt, arg_array, sizeof...(args));
    if (str.back() == '\n') {
      str.pop_back();
    }
    str.append(L"\x1b[0m\n");
    return bela::terminal::WriteAuto(stderr, str);
  }
  inline bela::ssize_t DbgPrint(const wchar_t *fmt) {
    if (!IsDebugMode) {
      return 0;
    }
    std::wstring_view msg(fmt);
    if (!msg.empty() && msg.back() == '\n') {
      msg.remove_suffix(1);
    }
    return bela::terminal::WriteAuto(stderr, bela::StringCat(L"\x1b[33m* ", msg, L"\x1b[0m\n"));
  }
  std::vector<PackageEnv> standardEnvs; // no package requires this
  std::list<PackageEnv> requiresEnvs;   // some package requires this
  int depth{0};
  bool IsDebugMode{false};
};
} // namespace baulk::env

#endif