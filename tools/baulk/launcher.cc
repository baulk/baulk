//
#include <bela/subsitute.hpp>
#include <bela/base.hpp>
#include <bela/path.hpp>
#include <bela/pe.hpp>
#include <bela/narrow/strcat.hpp>
#include <bela/io.hpp>
#include <bela/str_split.hpp>
#include <time.hpp>
#include <jsonex.hpp>
#include "launcher.hpp"
#include "fs.hpp"
#include "rcwriter.hpp"
// template
#include "launcher.template.ipp"

namespace baulk {

bool BaulkLinkMetaStore(const std::vector<LinkMeta> &metas, const Package &pkg, bela::error_code &ec) {
  if (metas.empty()) {
    return true;
  }
  auto linkmeta = bela::StringCat(baulk::BaulkRoot(), L"\\", baulk::BaulkLinkMeta);
  nlohmann::json obj;
  [&]() {
    FILE *fd = nullptr;
    if (auto eo = _wfopen_s(&fd, linkmeta.data(), L"rb"); eo != 0) {
      return;
    }
    auto closer = bela::finally([&] { fclose(fd); });
    try {
      obj = nlohmann::json::parse(fd, nullptr, true, true);
    } catch (const std::exception &e) {
      DbgPrint(L"Parse link meta json %s", e.what());
    }
  }();
  std::string newjson;
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
      auto meta = bela::ToNarrow(bela::StringCat(pkg.name, L"@", lm.path, L"@", pkg.version));
      newlinks[bela::ToNarrow(lm.alias)] = meta; // "7z.exe":"7z@7z.exe@19.01"
    }
    obj["links"] = newlinks;
    obj["updated"] = baulk::time::TimeNow();
    newjson = obj.dump(4);
  } catch (const std::exception &e) {
    ec = bela::make_error_code(1, bela::ToWide(e.what()));
    return false;
  }
  if (newjson.empty()) {
    return true;
  }
  return bela::io::WriteTextAtomic(newjson, linkmeta, ec);
}

bool BaulkRemovePkgLinks(std::wstring_view pkg, bela::error_code &ec) {
  auto linkmeta = bela::StringCat(baulk::BaulkRoot(), L"\\", baulk::BaulkLinkMeta);
  auto linkbindir = bela::StringCat(baulk::BaulkRoot(), L"\\", baulk::BaulkLinkDir);
  nlohmann::json obj;
  [&]() {
    FILE *fd = nullptr;
    if (auto eo = _wfopen_s(&fd, linkmeta.data(), L"rb"); eo != 0) {
      return;
    }
    auto closer = bela::finally([&] { fclose(fd); });
    try {
      obj = nlohmann::json::parse(fd, nullptr, true, true);
    } catch (const std::exception &) {
    }
  }();
  std::string newjson;
  std::error_code e;
  try {
    auto it = obj.find("links");
    if (it == obj.end()) {
      return true;
    }
    nlohmann::json newlinkobj;
    auto linksobj = it.value();
    for (auto it = linksobj.begin(); it != linksobj.end(); ++it) {
      auto raw = it.value().get<std::string>();
      auto value = bela::ToWide(raw);
      std::vector<std::wstring_view> mv = bela::StrSplit(value, bela::ByChar('@'), bela::SkipEmpty());
      if (mv.size() < 2) {
        continue;
      }
      if (mv[0] != pkg) {
        newlinkobj[it.key()] = raw;
        continue;
      }
      auto file = bela::StringCat(linkbindir, L"\\", bela::ToWide(it.key()));
      if (!std::filesystem::remove(file, e)) {
        auto ec = bela::from_std_error_code(e);
        baulk::DbgPrint(L"baulk remove link %s error: %s\n", file, ec.message);
      }
    }
    obj["updated"] = baulk::time::TimeNow();
    obj["links"] = newlinkobj;
    newjson = obj.dump(4);
  } catch (const std::exception &e) {
    ec = bela::make_error_code(1, bela::ToWide(e.what()));
    return false;
  }
  if (newjson.empty()) {
    return true;
  }
  return bela::io::WriteTextAtomic(newjson, linkmeta, ec);
}

// GenerateLinkSource generate link sources
std::wstring GenerateLinkSource(std::wstring_view target, bela::pe::Subsystem subs) {
  std::wstring escapetarget;
  escapetarget.reserve(target.size() + 10);
  for (auto c : target) {
    if (c == '\\') {
      escapetarget.append(L"\\\\");
      continue;
    }
    escapetarget.push_back(c);
  }
  if (subs == bela::pe::Subsystem::CUI) {
    return bela::Substitute(launcher_internal::consoletemplete, escapetarget);
  }
  return bela::Substitute(launcher_internal::windowstemplate, escapetarget);
}

class LinkExecutor {
public:
  LinkExecutor() = default;
  LinkExecutor(const LinkExecutor &) = delete;
  LinkExecutor &operator=(const LinkExecutor &) = delete;
  ~LinkExecutor() {
    if (!baulktemp.empty() && !baulk::IsTraceMode) {
      std::error_code ec;
      std::filesystem::remove_all(baulktemp, ec);
    }
  }
  bool Initialize(bela::error_code &ec);
  bool Compile(const baulk::Package &pkg, std::wstring_view source, std::wstring_view linkdir,
               const baulk::LinkMeta &lm, bela::error_code &ec);
  const std::vector<LinkMeta> &LinkMetas() const { return linkmetas; }

private:
  std::wstring baulktemp;
  std::vector<LinkMeta> linkmetas;
};

bool LinkExecutor::Initialize(bela::error_code &ec) {
  auto bktemp = baulk::fs::BaulkMakeTempDir(ec);
  if (!bktemp) {
    return false;
  }
  baulktemp.assign(std::move(*bktemp));
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

bool LinkExecutor::Compile(const baulk::Package &pkg, std::wstring_view source, std::wstring_view linkdir,
                           const baulk::LinkMeta &lm, bela::error_code &ec) {
  constexpr const std::wstring_view entry[] = {L"-ENTRY:wmain", L"-ENTRY:wWinMain"};
  constexpr const std::wstring_view subsystemnane[] = {L"-SUBSYSTEM:CONSOLE", L"-SUBSYSTEM:WINDOWS"};
  auto realexe = bela::RealPathEx(source, ec);
  if (!realexe) {
    return false;
  }
  auto pe = bela::pe::Expose(*realexe, ec);
  if (!pe) {
    // not pe subname
    return false;
  }
  auto index = SubsystemIndex(pe->subsystem);
  auto name = StripExtension(lm.alias);
  auto cxxsrcname = bela::StringCat(name, L".cc");
  auto cxxsrc = bela::StringCat(baulktemp, L"\\", cxxsrcname);
  auto rcsrcname = bela::StringCat(name, L".rc");
  auto rcsrc = bela::StringCat(baulktemp, L"\\", rcsrcname);
  if (!bela::io::WriteText(GenerateLinkSource(source, pe->subsystem), cxxsrc, ec)) {
    return false;
  }
  bool rcwrited = false;
  if (auto vi = bela::pe::ExposeVersion(source, ec); vi) {
    baulk::rc::Writer w;
    if (vi->CompanyName.empty()) {
      vi->CompanyName = bela::StringCat(pkg.name, L" contributors");
    }
    StringNonEmpty(vi->FileDescription, pkg.description);
    StringNonEmpty(vi->FileVersion, pkg.version);
    StringNonEmpty(vi->ProductVersion, pkg.version);
    StringNonEmpty(vi->ProductName, pkg.name);
    StringNonEmpty(vi->OriginalFileName, lm.alias);
    StringNonEmpty(vi->InternalName, lm.alias);
    rcwrited = w.WriteVersion(*vi, rcsrc, ec);
  } else {
    bela::pe::VersionInfo nvi;
    nvi.CompanyName = bela::StringCat(pkg.name, L" contributors");
    nvi.FileDescription = pkg.description;
    nvi.FileVersion = pkg.version;
    nvi.ProductVersion = pkg.version;
    nvi.ProductName = pkg.name;
    nvi.OriginalFileName = lm.alias;
    nvi.InternalName = lm.alias;
    baulk::rc::Writer w;
    rcwrited = w.WriteVersion(nvi, rcsrc, ec);
  }
  if (rcwrited) {
    if (baulk::BaulkExecutor().Execute(baulktemp, L"rc", L"-nologo", L"-c65001", rcsrcname) != 0) {
      rcwrited = false;
    }
  }
  DbgPrint(L"compile %s", cxxsrcname);
  if (baulk::BaulkExecutor().Execute(baulktemp, L"cl", L"-c", L"-std:c++17", L"-nologo", L"-Os", cxxsrcname) != 0) {
    ec = baulk::BaulkExecutor().LastErrorCode();
    return false;
  }
  int exitcode = 0;
  DbgPrint(L"link %s.obj rcwrited: %b", name, rcwrited);
  if (rcwrited) {
    exitcode = baulk::BaulkExecutor().Execute(baulktemp, L"link", L"-nologo", L"-OPT:REF", L"-OPT:ICF",
                                              L"-NODEFAULTLIB", subsystemnane[index], entry[index],
                                              bela::StringCat(name, L".obj"), bela::StringCat(name, L".res"),
                                              L"kernel32.lib", L"user32.lib", bela::StringCat(L"-OUT:", lm.alias));
  } else {
    exitcode = baulk::BaulkExecutor().Execute(
        baulktemp, L"link", L"-nologo", L"-OPT:REF", L"-OPT:ICF", L"-NODEFAULTLIB", subsystemnane[index], entry[index],
        bela::StringCat(name, L".obj"), L"kernel32.lib", L"user32.lib", bela::StringCat(L"-OUT:", lm.alias));
  }
  if (exitcode != 0) {
    ec = baulk::BaulkExecutor().LastErrorCode();
    return false;
  }
  auto target = bela::StringCat(linkdir, L"\\", lm.alias);
  auto mktarget = bela::StringCat(baulktemp, L"\\", lm.alias);
  std::error_code e;
  if (std::filesystem::exists(target, e)) {
    std::filesystem::remove_all(target, e);
  }
  std::filesystem::rename(mktarget, target, e);
  if (e) {
    ec = bela::from_std_error_code(e);
    return false;
  }
  linkmetas.emplace_back(lm);
  return true;
}

bool MakeLaunchers(const baulk::Package &pkg, bool forceoverwrite, bela::error_code &ec) {
  auto pkgroot = bela::StringCat(baulk::BaulkRoot(), L"\\", BaulkPkgsDir, L"\\", pkg.name);
  auto linkdir = bela::StringCat(baulk::BaulkRoot(), L"\\", BaulkLinkDir);
  if (!baulk::fs::MakeDir(linkdir, ec)) {
    return false;
  }
  LinkExecutor executor;
  if (!executor.Initialize(ec)) {
    return false;
  }
  for (const auto &lm : pkg.launchers) {
    auto source = bela::PathCat(pkgroot, L"\\", lm.path);
    DbgPrint(L"make launcher %s", source);
    if (!executor.Compile(pkg, source, linkdir, lm, ec)) {
      bela::FPrintF(stderr, L"'%s' unable create launcher: \x1b[31m%s\x1b[0m\n", source, ec.message);
    }
  }
  if (!BaulkLinkMetaStore(executor.LinkMetas(), pkg, ec)) {
    bela::FPrintF(stderr,
                  L"%s create links error: %s\nYour can run 'baulk uninstall' "
                  L"and retry\n",
                  pkg.name, ec.message);
    return false;
  }
  return true;
}

bool MakeSimulatedLauncher(const baulk::Package &pkg, bool forceoverwrite, bela::error_code &ec) {
  auto baulklnk = bela::StringCat(baulk::BaulkRoot(), L"\\bin\\baulk-lnk.exe");
  if (!bela::PathExists(baulklnk)) {
    ec = bela::make_error_code(1, L"baulk-lnk not exists. cannot create simulated launcher");
    return false;
  }
  auto pkgroot = bela::StringCat(baulk::BaulkRoot(), L"\\", BaulkPkgsDir, L"\\", pkg.name);
  auto linkdir = bela::StringCat(baulk::BaulkRoot(), L"\\", BaulkLinkDir);
  if (!baulk::fs::MakeDir(linkdir, ec)) {
    return false;
  }
  std::vector<LinkMeta> linkmetas;
  for (const auto &lm : pkg.links) {
    auto src = bela::PathCat(pkgroot, L"\\", lm.path);
    if (!bela::PathExists(src)) {
      bela::FPrintF(stderr, L"%s not exist\n", src);
      continue;
    }
    auto lnk = bela::StringCat(linkdir, L"\\", lm.alias);
    if (bela::PathExists(lnk)) {
      if (forceoverwrite) {
        baulk::fs::PathRemove(lnk, ec);
      }
    }
    // use baulk-cli as source
    if (!baulk::fs::SymLink(baulklnk, lnk, ec)) {
      return false;
    }
    linkmetas.emplace_back(lm);
  }
  if (!BaulkLinkMetaStore(linkmetas, pkg, ec)) {
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
  auto pkgroot = bela::StringCat(baulk::BaulkRoot(), L"\\", BaulkPkgsDir, L"\\", pkg.name);
  auto linkdir = bela::StringCat(baulk::BaulkRoot(), L"\\", BaulkLinkDir);
  if (!baulk::fs::MakeDir(linkdir, ec)) {
    return false;
  }
  std::vector<LinkMeta> linkmetas;
  for (const auto &lm : pkg.links) {
    auto src = bela::PathCat(pkgroot, L"\\", lm.path);
    if (!bela::PathExists(src)) {
      bela::FPrintF(stderr, L"%s not exist\n", src);
      continue;
    }
    auto lnk = bela::StringCat(linkdir, L"\\", lm.alias);
    if (bela::PathExists(lnk)) {
      if (forceoverwrite) {
        baulk::fs::PathRemove(lnk, ec);
      }
    }
    if (!baulk::fs::SymLink(src, lnk, ec)) {
      DbgPrint(L"make %s -> %s: %s\n", src, lnk, ec.message);
      return false;
    }
    linkmetas.emplace_back(lm);
  }
  if (!BaulkLinkMetaStore(linkmetas, pkg, ec)) {
    bela::FPrintF(stderr,
                  L"%s create links error: %s\nYour can run 'baulk uninstall' "
                  L"and retry\n",
                  pkg.name, ec.message);
    return false;
  }
  return true;
}

bool BaulkMakePkgLinks(const baulk::Package &pkg, bool forceoverwrite, bela::error_code &ec) {
  if (!pkg.links.empty() && !MakeSymlinks(pkg, forceoverwrite, ec)) {
    return false;
  }
  if (pkg.launchers.empty()) {
    return true;
  }
  if (!baulk::BaulkExecutor().Initialized()) {
    return MakeSimulatedLauncher(pkg, forceoverwrite, ec);
  }
  return MakeLaunchers(pkg, forceoverwrite, ec);
}
} // namespace baulk
