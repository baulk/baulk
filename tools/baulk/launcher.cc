//
#include <bela/subsitute.hpp>
#include <bela/base.hpp>
#include <bela/path.hpp>
#include <bela/pe.hpp>
#include <bela/str_cat_narrow.hpp>
#include <bela/io.hpp>
#include <bela/str_split.hpp>
#include <bela/datetime.hpp>
#include <baulk/fs.hpp>
#include <baulk/vfs.hpp>
#include <baulk/json_utils.hpp>
#include <baulk/hash.hpp>
#include "launcher.hpp"
#include "rcwriter.hpp"
// template
#include "launcher.template.ipp"

namespace baulk {

bool LinkMetaStore(const std::vector<LinkMeta> &metas, const Package &pkg, bela::error_code &ec) {
  if (metas.empty()) {
    return true;
  }
  auto linkMeta = bela::StringCat(vfs::AppLinks(), L"\\baulk.linkmeta.json");
  nlohmann::json obj;
  if (auto jv = json::parse_file(linkMeta, ec); jv) {
    obj = std::move(jv->obj);
  }
  try {
    nlohmann::json newlinks;
    if (auto it = obj.find("links"); it != obj.end()) {
      for (const auto &item : it.value().items()) {
        if (item.value().is_string()) {
          newlinks[item.key()] = item.value().get<std::string>();
        }
      }
    }
    for (const auto &lm : metas) {
      auto meta = bela::encode_into<wchar_t, char>(bela::StringCat(pkg.name, L"@", lm.path, L"@", pkg.version));
      newlinks[bela::encode_into<wchar_t, char>(lm.alias)] = meta; // "7z.exe":"7z@7z.exe@19.01"
    }
    obj["links"] = newlinks;
    obj["updated"] = bela::FormatTime<char>(bela::Now());
    obj["app_packages_root"] = bela::encode_into<wchar_t, char>(vfs::AppPackages());
    if (!bela::io::WriteTextAtomic(obj.dump(4), linkMeta, ec)) {
      return false;
    }
  } catch (const std::exception &e) {
    ec = bela::make_error_code(bela::ErrGeneral, bela::encode_into<char, wchar_t>(e.what()));
    return false;
  }
  return true;
}

bool RemovePackageLinks(std::wstring_view pkgName, bela::error_code &ec) {
  auto linkMeta = bela::StringCat(vfs::AppLinks(), L"\\baulk.linkmeta.json");
  auto appLinks = vfs::AppLinks();
  nlohmann::json obj;
  if (auto jv = json::parse_file(linkMeta, ec); jv) {
    obj = std::move(jv->obj);
  }
  std::string newjson;
  std::error_code e;
  try {
    auto it = obj.find("links");
    if (it == obj.end()) {
      return true;
    }
    nlohmann::json newlinks;
    auto links = it.value();
    for (auto link = links.begin(); link != links.end(); ++link) {
      auto raw = link.value().get<std::string>();
      auto value = bela::encode_into<char, wchar_t>(raw);
      std::vector<std::wstring_view> mv = bela::StrSplit(value, bela::ByChar('@'), bela::SkipEmpty());
      if (mv.size() < 2) {
        continue;
      }
      if (mv[0] != pkgName) {
        newlinks[link.key()] = raw;
        continue;
      }
      auto file = bela::StringCat(appLinks, L"\\", bela::encode_into<char, wchar_t>(link.key()));
      if (!std::filesystem::remove(file, e)) {
        auto le = bela::from_std_error_code(e);
        baulk::DbgPrint(L"baulk remove link %s error: %s\n", file, le.message);
      }
    }
    obj["updated"] = bela::FormatTime<char>(bela::Now());
    obj["links"] = newlinks;
    if (!bela::io::WriteTextAtomic(obj.dump(4), linkMeta, ec)) {
      return false;
    }
  } catch (const std::exception &e) {
    ec = bela::make_error_code(bela::ErrGeneral, bela::encode_into<char, wchar_t>(e.what()));
    return false;
  }
  return true;
}

// GenerateLinkSource generate link sources
std::wstring GenerateLinkSource(std::wstring_view target, bool isConsole) {
  std::wstring escapetarget;
  escapetarget.reserve(target.size() + 10);
  for (auto c : target) {
    if (c == '\\') {
      escapetarget.append(L"\\\\");
      continue;
    }
    escapetarget.push_back(c);
  }
  if (isConsole) {
    return bela::Substitute(launcher_internal::consoletemplete, escapetarget);
  }
  return bela::Substitute(launcher_internal::windowstemplate, escapetarget);
}

class Builder {
public:
  Builder() = default;
  Builder(const Builder &) = delete;
  Builder &operator=(const Builder &) = delete;
  ~Builder() {
    if (!buildTemp.empty() && !baulk::IsTraceMode) {
      std::error_code ec;
      std::filesystem::remove_all(buildTemp, ec);
    }
  }
  bool Initialize(bela::error_code &ec);
  bool Compile(const baulk::Package &pkg, std::wstring_view source, std::wstring_view appLinks,
               const baulk::LinkMeta &linkMeta, bela::error_code &ec);
  const std::vector<LinkMeta> &LinkMetas() const { return linkmetas; }

private:
  std::wstring buildTemp;
  std::vector<LinkMeta> linkmetas;
};

bool Builder::Initialize(bela::error_code &ec) {
  auto bktemp = baulk::fs::NewTempFolder(ec);
  if (!bktemp) {
    return false;
  }
  buildTemp.assign(std::move(*bktemp));
  return true;
}

inline int SubsystemIndex(bela::pe::Subsystem subs) {
  if (subs == bela::pe::Subsystem::CUI) {
    return 0;
  }
  return 1;
}

inline std::wstring_view StripExtension(std::wstring_view filename) {
  auto pos = filename.rfind('.');
  if (pos == std::wstring_view::npos) {
    return filename;
  }
  return filename.substr(0, pos);
}

inline void StringNonEmpty(std::wstring &s, std::wstring_view d) {
  if (s.empty()) {
    s = d;
  }
}

bool Builder::Compile(const baulk::Package &pkg, std::wstring_view source, std::wstring_view appLinks,
                      const baulk::LinkMeta &linkMeta, bela::error_code &ec) {
  constexpr const std::wstring_view entry[] = {L"-ENTRY:wmain", L"-ENTRY:wWinMain"};
  constexpr const std::wstring_view subsyetmName[] = {L"-SUBSYSTEM:CONSOLE", L"-SUBSYSTEM:WINDOWS"};
  auto realexe = bela::RealPathEx(source, ec);
  if (!realexe) {
    return false;
  }
  auto isConsole = bela::pe::IsSubsystemConsole(*realexe);
  DbgPrint(L"executable %s is subsystem console: %v\n", *realexe, isConsole);
  auto name = StripExtension(linkMeta.alias);
  auto cxxSourceName = bela::StringCat(name, L".cc");
  auto cxxSourcePath = bela::StringCat(buildTemp, L"\\", cxxSourceName);
  auto rcSourceName = bela::StringCat(name, L".rc");
  auto rcSourcePath = bela::StringCat(buildTemp, L"\\", rcSourceName);
  if (!bela::io::WriteText(GenerateLinkSource(source, isConsole), cxxSourcePath, ec)) {
    return false;
  }
  bool rcwrited = false;
  if (auto vi = bela::pe::Lookup(source, ec); vi) {
    baulk::rc::Writer w;
    if (vi->CompanyName.empty()) {
      vi->CompanyName = bela::StringCat(pkg.name, L" contributors");
    }
    StringNonEmpty(vi->FileDescription, pkg.description);
    StringNonEmpty(vi->FileVersion, pkg.version);
    StringNonEmpty(vi->ProductVersion, pkg.version);
    StringNonEmpty(vi->ProductName, pkg.name);
    StringNonEmpty(vi->OriginalFileName, linkMeta.alias);
    StringNonEmpty(vi->InternalName, linkMeta.alias);
    rcwrited = w.WriteVersion(*vi, rcSourcePath, ec);
  } else {
    bela::pe::Version nvi;
    nvi.CompanyName = bela::StringCat(pkg.name, L" contributors");
    nvi.FileDescription = pkg.description;
    nvi.FileVersion = pkg.version;
    nvi.ProductVersion = pkg.version;
    nvi.ProductName = pkg.name;
    nvi.OriginalFileName = linkMeta.alias;
    nvi.InternalName = linkMeta.alias;
    baulk::rc::Writer w;
    rcwrited = w.WriteVersion(nvi, rcSourcePath, ec);
  }
  if (rcwrited) {
    if (LinkExecutor().Execute(buildTemp, L"rc", L"-nologo", L"-c65001", rcSourceName) != 0) {
      rcwrited = false;
    }
  }
  DbgPrint(L"compile %s [%s]", cxxSourceName, buildTemp);
  if (LinkExecutor().Execute(buildTemp, L"cl", L"-c", L"-std:c++17", L"-nologo", L"-Os", cxxSourceName) != 0) {
    ec = LinkExecutor().LastErrorCode();
    return false;
  }
  int exitcode = 0;
  DbgPrint(L"link %s.obj rcwrited: %b", name, rcwrited);
  auto index = isConsole ? 0 : 1;
  if (rcwrited) {
    exitcode = LinkExecutor().Execute(buildTemp, L"link", L"-nologo", L"-OPT:REF", L"-OPT:ICF", L"-NODEFAULTLIB",
                                      subsyetmName[index], entry[index], bela::StringCat(name, L".obj"),
                                      bela::StringCat(name, L".res"), L"kernel32.lib", L"user32.lib",
                                      bela::StringCat(L"-OUT:", linkMeta.alias));
  } else {
    exitcode = LinkExecutor().Execute(buildTemp, L"link", L"-nologo", L"-OPT:REF", L"-OPT:ICF", L"-NODEFAULTLIB",
                                      subsyetmName[index], entry[index], bela::StringCat(name, L".obj"),
                                      L"kernel32.lib", L"user32.lib", bela::StringCat(L"-OUT:", linkMeta.alias));
  }
  if (exitcode != 0) {
    ec = LinkExecutor().LastErrorCode();
    return false;
  }
  auto target = bela::StringCat(appLinks, L"\\", linkMeta.alias);
  auto genTarget = bela::StringCat(buildTemp, L"\\", linkMeta.alias);
  std::error_code e;
  if (std::filesystem::exists(target, e)) {
    std::filesystem::remove_all(target, e);
  }
  std::filesystem::rename(genTarget, target, e);
  if (e) {
    ec = bela::from_std_error_code(e);
    return false;
  }
  linkmetas.emplace_back(linkMeta);
  return true;
}

bool MakeLaunchers(const baulk::Package &pkg, bool forceoverwrite, bela::error_code &ec) {
  auto packageRoot = vfs::AppPackageFolder(pkg.name);
  auto appLinks = vfs::AppLinks();
  if (!baulk::fs::MakeDir(appLinks, ec)) {
    return false;
  }
  Builder builder;
  if (!builder.Initialize(ec)) {
    return false;
  }
  for (const auto &lm : pkg.launchers) {
    auto source = bela::PathCat(packageRoot, L"\\", lm.path);
    DbgPrint(L"make launcher %s", source);
    if (!builder.Compile(pkg, source, appLinks, lm, ec)) {
      bela::FPrintF(stderr, L"unable create launcher '%s': \x1b[31m%s\x1b[0m\n", bela::BaseName(source), ec.message);
    }
  }
  if (!LinkMetaStore(builder.LinkMetas(), pkg, ec)) {
    bela::FPrintF(stderr,
                  L"%s create links error: %s\nYour can run 'baulk uninstall' "
                  L"and retry\n",
                  pkg.name, ec.message);
    return false;
  }
  return true;
}

std::optional<std::wstring> FindProxyLauncher(bela::error_code &ec) {
  if (!vfs::IsPackaged()) {
    auto proxyLauncher = bela::StringCat(vfs::AppExecutableRoot(), L"\\bin\\baulk-lnk.exe");
    if (!bela::PathExists(proxyLauncher)) {
      ec = bela::make_error_code(bela::ErrGeneral, L"baulk-lnk.exe not found");
      return std::nullopt;
    }
    return std::make_optional(std::move(proxyLauncher));
  }
  auto proxyLauncher = bela::StringCat(vfs::AppExecutableRoot(), L"\\bin\\baulk-lnk.exe");
  auto localLauncher = bela::StringCat(vfs::AppBasePath(), L"\\bin\\baulk-lnk.exe");
  auto h = hash::FileHash(proxyLauncher, hash::hash_t::BLAKE3, ec);
  if (!h) {
    ec.message = bela::StringCat(L"baulk-lnk.exe not found error: ", ec.message);
    return std::nullopt;
  }
  if (auto h2 = hash::FileHash(localLauncher, hash::hash_t::BLAKE3, ec); h2) {
    if (*h == *h2) {
      return std::make_optional(std::move(localLauncher));
    }
  }
  if (CopyFileW(proxyLauncher.data(), localLauncher.data(), TRUE) != TRUE) {
    ec = bela::make_system_error_code(L"replace local proxy launcher: ");
    return std::nullopt;
  }
  return std::make_optional(std::move(localLauncher));
}

bool MakeProxyLaunchers(const baulk::Package &pkg, bool forceoverwrite, bela::error_code &ec) {
  auto proxyLauncher = FindProxyLauncher(ec);
  if (!proxyLauncher) {
    return false;
  }
  auto packageRoot = vfs::AppPackageFolder(pkg.name);
  auto appLinks = vfs::AppLinks();
  if (!baulk::fs::MakeDir(appLinks, ec)) {
    return false;
  }
  std::vector<LinkMeta> linkmetas;
  for (const auto &lm : pkg.launchers) {
    auto src = bela::PathCat(packageRoot, L"\\", lm.path);
    DbgPrint(L"make simulated launcher %s", src);
    if (!bela::PathExists(src)) {
      bela::FPrintF(stderr, L"%s not exist\n", src);
      continue;
    }
    auto lnk = bela::StringCat(appLinks, L"\\", lm.alias);
    if (bela::PathExists(lnk)) {
      if (forceoverwrite) {
        bela::fs::ForceDeleteFile(lnk, ec);
      }
    }
    // use baulk-lnk.exe as proxy
    if (!baulk::fs::SymLink(*proxyLauncher, lnk, ec)) {
      return false;
    }
    linkmetas.emplace_back(lm);
  }
  if (!LinkMetaStore(linkmetas, pkg, ec)) {
    bela::FPrintF(stderr,
                  L"%s create links error: %s\nYour can run 'baulk uninstall' "
                  L"and retry\n",
                  pkg.name, ec.message);
    return false;
  }
  return true;
}

// create symlink
bool MakeSymlinks(const baulk::Package &pkg, bool forceoverwrite, bela::error_code &ec) {
  auto packageRoot = vfs::AppPackageFolder(pkg.name);
  auto appLinks = vfs::AppLinks();
  if (!baulk::fs::MakeDir(appLinks, ec)) {
    return false;
  }
  std::vector<LinkMeta> linkmetas;
  for (const auto &lm : pkg.links) {
    auto src = bela::PathCat(packageRoot, L"\\", lm.path);
    DbgPrint(L"make symlink %s", src);
    if (!bela::PathExists(src)) {
      bela::FPrintF(stderr, L"%s not exist\n", src);
      continue;
    }
    auto lnk = bela::StringCat(appLinks, L"\\", lm.alias);
    if (bela::PathExists(lnk)) {
      if (forceoverwrite) {
        bela::fs::ForceDeleteFolders(lnk, ec);
      }
    }
    if (!baulk::fs::SymLink(src, lnk, ec)) {
      DbgPrint(L"make %s -> %s: %s\n", src, lnk, ec.message);
      return false;
    }
    linkmetas.emplace_back(lm);
  }
  if (!LinkMetaStore(linkmetas, pkg, ec)) {
    bela::FPrintF(stderr,
                  L"%s create links error: %s\nYour can run 'baulk uninstall' "
                  L"and retry\n",
                  pkg.name, ec.message);
    return false;
  }
  return true;
}

bool MakePackageLinks(const baulk::Package &pkg, bool forceoverwrite, bela::error_code &ec) {
  if (!pkg.links.empty()) {
    if (!MakeSymlinks(pkg, forceoverwrite, ec)) {
      return false;
    }
  }
  if (pkg.launchers.empty()) {
    return true;
  }
  if (!LinkExecutor().Initialized()) {
    return MakeProxyLaunchers(pkg, forceoverwrite, ec);
  }
  return MakeLaunchers(pkg, forceoverwrite, ec);
}
} // namespace baulk
