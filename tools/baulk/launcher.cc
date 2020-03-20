//
#include <bela/subsitute.hpp>
#include <bela/finaly.hpp>
#include <bela/path.hpp>
#include <bela/pe.hpp>
#include <bela/narrow/strcat.hpp>
#include "launcher.hpp"
#include "fs.hpp"
#include "rcwriter.hpp"
#include "jsonex.hpp"
#include "launcher.template.ipp"

namespace baulk {

struct LinkMeta {
  LinkMeta() = default;
  LinkMeta(std::wstring_view exe_, std::wstring_view rexe_)
      : exe(exe_), relative(rexe_) {}
  std::wstring exe;
  std::wstring relative;
};

bool BaulkLinkMetaStore(const std::vector<LinkMeta> metas, const Package &pkg,
                        bela::error_code &ec) {
  if (metas.empty()) {
    return true;
  }
  auto linkmeta =
      bela::StringCat(baulk::BaulkRoot(), L"\\", baulk::BaulkLinkMeta);
  nlohmann::json obj;
  [&]() {
    FILE *fd = nullptr;
    if (auto eo = _wfopen_s(&fd, linkmeta.data(), L"rb"); eo != 0) {
      return;
    }
    auto closer = bela::finally([&] { fclose(fd); });
    try {
      obj = nlohmann::json::parse(fd);
    } catch (const std::exception &) {
    }
  }();
  std::string newjson;
  try {
    auto links = obj.find("links");
    nlohmann::json linkobj;
    if (links != obj.end()) {
      linkobj = links.value();
    }
    for (const auto &lm : metas) {
      auto meta = bela::ToNarrow(
          bela::StringCat(pkg.name, L"@", lm.relative, L"@", pkg.version));
      linkobj[bela::ToNarrow(lm.exe)] = meta; // "7z.exe":"7z.exe@7z@19.01"
    }
    obj["links"] = linkobj;
    newjson = obj.dump(4);
  } catch (const std::exception &e) {
    ec = bela::make_error_code(1, bela::ToWide(e.what()));
    return false;
  }
  if (newjson.empty()) {
    return true;
  }
  return baulk::io::WriteTextAtomic(newjson, linkmeta, ec);
}

bool BaulkRemovePkgLinks(std::wstring_view pkg, bela::error_code &ec) {
  auto linkmeta =
      bela::StringCat(baulk::BaulkRoot(), L"\\", baulk::BaulkLinkMeta);
  auto linkbindir =
      bela::StringCat(baulk::BaulkRoot(), L"\\", baulk::BaulkLinkDir);
  nlohmann::json obj;
  [&]() {
    FILE *fd = nullptr;
    if (auto eo = _wfopen_s(&fd, linkmeta.data(), L"rb"); eo != 0) {
      return;
    }
    auto closer = bela::finally([&] { fclose(fd); });
    try {
      obj = nlohmann::json::parse(fd);
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
      auto value = it.value().get<std::string_view>();
      std::vector<std::wstring_view> mv = bela::StrSplit(
          bela::ToWide(value), bela::ByChar('@'), bela::SkipEmpty());
      if (mv.size() < 2) {
        continue;
      }
      if (mv[0] != pkg) {
        newlinkobj[it.key()] = value;
        continue;
      }
      auto file = bela::StringCat(linkbindir, L"\\", bela::ToWide(it.key()));
      if (!std::filesystem::remove(file, e)) {
        bela::FPrintF(stderr, L"baulk remove link %s error: %s\n", file,
                      e.message());
      }
    }
    obj["links"] = newlinkobj;
    newjson = obj.dump(4);
  } catch (const std::exception &e) {
    ec = bela::make_error_code(1, bela::ToWide(e.what()));
    return false;
  }
  if (newjson.empty()) {
    return true;
  }
  return baulk::io::WriteTextAtomic(newjson, linkmeta, ec);
}

// GenerateLinkSource generate link sources
std::wstring GenerateLinkSource(std::wstring_view target,
                                bela::pe::Subsystem subs) {
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
    if (!baulktemp.empty()) {
      std::error_code ec;
      std::filesystem::remove_all(baulktemp, ec);
    }
  }
  bool Initialize(bela::error_code &ec);
  bool Compile(const baulk::Package &pkg, std::wstring_view source,
               std::wstring_view linkdir, std::wstring_view relativeexe,
               bela::error_code &ec);
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

bool LinkExecutor::Compile(const baulk::Package &pkg, std::wstring_view source,
                           std::wstring_view linkdir,
                           std::wstring_view relativeexe,
                           bela::error_code &ec) {
  //      bela::FPrintF(stderr,L"link '%s' to '%s' success\n",source,linkdir);
  constexpr const std::wstring_view entry[] = {L"-ENTRY:wmain",
                                               L"-ENTRY:wWinMain"};
  constexpr const std::wstring_view subsystemnane[] = {L"-SUBSYSTEM:CONSOLE",
                                                       L"-SUBSYSTEM:WINDOWS"};
  auto pe = bela::pe::Expose(source, ec);
  if (!pe) {
    // not pe subname
    return false;
  }
  auto index = SubsystemIndex(pe->subsystem);
  auto exename = baulk::fs::FileName(source);
  auto name = StripExtension(exename);
  auto cxxsrcname = bela::StringCat(name, L".cc");
  auto cxxsrc = bela::StringCat(baulktemp, L"\\", cxxsrcname);
  auto rcsrcname = bela::StringCat(name, L".rc");
  auto rcsrc = bela::StringCat(baulktemp, L"\\", rcsrcname);
  if (baulk::io::WriteText(GenerateLinkSource(source, pe->subsystem), cxxsrc,
                           ec)) {
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
    StringNonEmpty(vi->OriginalFileName, exename);
    StringNonEmpty(vi->InternalName, exename);
    rcwrited = w.WriteVersion(*vi, rcsrc, ec);
  } else {
    bela::pe::VersionInfo nvi;
    nvi.CompanyName = bela::StringCat(pkg.name, L" contributors");
    nvi.FileDescription = pkg.description;
    nvi.FileVersion = pkg.version;
    nvi.ProductVersion = pkg.version;
    nvi.ProductName = pkg.name;
    nvi.OriginalFileName = exename;
    nvi.InternalName = exename;
    baulk::rc::Writer w;
    rcwrited = w.WriteVersion(nvi, rcsrc, ec);
  }
  if (rcwrited) {
    if (!baulk::BaulkExecutor().Execute(baulktemp, L"rc", L"-nologo",
                                        L"-c65001", rcsrcname)) {
      rcwrited = false;
    }
  }
  if (baulk::BaulkExecutor().Execute(baulktemp, L"cl", L"-c", L"-std:c++17",
                                     L"-nologo", L"-Os", cxxsrcname) != 0) {
    // compiler failed
    return false;
  }
  int exitcode = 0;
  if (rcwrited) {
    exitcode = baulk::BaulkExecutor().Execute(
        baulktemp, L"link", L"-nologo", L"-OPT:REF", L"-OPT:ICF",
        L"-NODEFAULTLIB", subsystemnane[index], entry[index],
        bela::StringCat(name, L".obj"), bela::StringCat(name, L".res"),
        L"kernel32.lib", L"user32.lib", bela::StringCat(L"-OUT:", exename));
  } else {
    exitcode = baulk::BaulkExecutor().Execute(
        baulktemp, L"link", L"-nologo", L"-OPT:REF", L"-OPT:ICF",
        L"-NODEFAULTLIB", subsystemnane[index], entry[index],
        bela::StringCat(cxxsrcname, L".obj"), L"kernel32.lib", L"user32.lib",
        bela::StringCat(L"-OUT:", exename));
  }
  if (exitcode != 0) {
    ec = baulk::BaulkExecutor().LastErrorCode();
    return false;
  }
  auto target = bela::StringCat(linkdir, L"\\", exename);
  auto newtarget = bela::StringCat(baulktemp, L"\\", exename);
  std::error_code e;
  if (std::filesystem::exists(target, e)) {
    std::filesystem::remove_all(target, e);
  }
  std::filesystem::rename(newtarget, target, e);
  if (!e) {
    ec = bela::from_std_error_code(e);
    return false;
  }
  linkmetas.emplace_back(exename, relativeexe);
  return true;
}

bool MakeSimulatedLauncher(const baulk::Package &pkg, bool forceoverwrite,
                           bela::error_code &ec) {
  auto baulkcli = bela::StringCat(baulk::BaulkRoot(), L"\\bin\\baulkcli.exe");
  if (!bela::PathExists(baulkcli)) {
    ec = bela::make_error_code(
        1, L"baulkcli not exists. cannot create simulated launcher");
    return false;
  }
  auto pkgroot =
      bela::StringCat(baulk::BaulkRoot(), L"\\", BaulkPkgsDir, L"\\", pkg.name);
  auto linkdir = bela::StringCat(baulk::BaulkRoot(), L"\\", BaulkLinkDir);
  std::vector<LinkMeta> linkmetas;
  for (const auto &lnk : pkg.links) {
    auto src = bela::PathCat(pkgroot, L"\\", lnk);
    if (!bela::PathExists(src)) {
      bela::FPrintF(stderr, L"%s not exist\n", src);
      continue;
    }
    auto fn = baulk::fs::FileName(src);
    auto lnk = bela::StringCat(linkdir, L"\\", fn);
    if (bela::PathExists(lnk)) {
      if (forceoverwrite) {
        baulk::fs::PathRemove(lnk, ec);
      }
    }
    if (!baulk::fs::SymLink(src, lnk, ec)) {
      return false;
    }
    linkmetas.emplace_back(baulkcli, lnk);
  }
  if (BaulkLinkMetaStore(linkmetas, pkg, ec)) {
    bela::FPrintF(stderr,
                  L"%s create links error: %s\nYour can run 'baulk uninstall' "
                  L"and retry\n",
                  pkg.name, ec.message);
    return false;
  }
  return true;
}

bool MakeLaunchers(const baulk::Package &pkg, bool forceoverwrite,
                   bela::error_code &ec) {
  auto pkgroot =
      bela::StringCat(baulk::BaulkRoot(), L"\\", BaulkPkgsDir, L"\\", pkg.name);
  auto linkdir = bela::StringCat(baulk::BaulkRoot(), L"\\", BaulkLinkDir);
  LinkExecutor executor;
  if (!executor.Initialize(ec)) {
    return false;
  }
  for (const auto &n : pkg.launchers) {
    auto source = bela::PathCat(pkgroot, n);
    if (!executor.Compile(pkg, source, linkdir, n, ec)) {
      bela::FPrintF(stderr, L"'%s' unable create launcher: \x1b[31m%s\x1b[0m\n",
                    source, ec.message);
    }
  }
  if (BaulkLinkMetaStore(executor.LinkMetas(), pkg, ec)) {
    bela::FPrintF(stderr,
                  L"%s create links error: %s\nYour can run 'baulk uninstall' "
                  L"and retry\n",
                  pkg.name, ec.message);
    return false;
  }
  return true;
}

// create symlink
bool MakeSymlinks(const baulk::Package &pkg, bool forceoverwrite,
                  bela::error_code &ec) {
  auto pkgroot =
      bela::StringCat(baulk::BaulkRoot(), L"\\", BaulkPkgsDir, L"\\", pkg.name);
  auto linkdir = bela::StringCat(baulk::BaulkRoot(), L"\\", BaulkLinkDir);
  std::vector<LinkMeta> linkmetas;
  for (const auto &lnk : pkg.links) {
    auto src = bela::PathCat(pkgroot, L"\\", lnk);
    if (!bela::PathExists(src)) {
      bela::FPrintF(stderr, L"%s not exist\n", src);
      continue;
    }
    auto fn = baulk::fs::FileName(src);
    auto lnk = bela::StringCat(linkdir, L"\\", fn);
    if (bela::PathExists(lnk)) {
      if (forceoverwrite) {
        baulk::fs::PathRemove(lnk, ec);
      }
    }
    if (!baulk::fs::SymLink(src, lnk, ec)) {
      return false;
    }
    linkmetas.emplace_back(fn, lnk);
  }
  if (BaulkLinkMetaStore(linkmetas, pkg, ec)) {
    bela::FPrintF(stderr,
                  L"%s create links error: %s\nYour can run 'baulk uninstall' "
                  L"and retry\n",
                  pkg.name, ec.message);
    return false;
  }
  return true;
}

bool BaulkMakePkgLinks(const baulk::Package &pkg, bool forceoverwrite,
                       bela::error_code &ec) {
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
