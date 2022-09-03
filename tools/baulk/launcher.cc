//
#include <chrono>
#include <bela/subsitute.hpp>
#include <bela/base.hpp>
#include <bela/path.hpp>
#include <bela/pe.hpp>
#include <bela/str_cat.hpp>
#include <bela/io.hpp>
#include <bela/str_split.hpp>
#include <bela/datetime.hpp>
#include <baulk/fs.hpp>
#include <baulk/vfs.hpp>
#include <baulk/json_utils.hpp>
#include <baulk/hash.hpp>
#include "launcher.hpp"
#include "generated.hpp"

namespace baulk {

bool LinkMetaStore(const std::vector<LinkMeta> &metas, const Package &pkg, bela::error_code &ec) {
  if (metas.empty()) {
    return true;
  }
  auto linkMeta = bela::StringCat(vfs::AppLinks(), L"\\baulk.linkmeta.json");
  nlohmann::json obj;
  if (auto jv = parse_json_file(linkMeta, ec); jv) {
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
    DbgPrint(L"write link meta: %v", linkMeta);
    if (!bela::io::AtomicWriteText(linkMeta, bela::io::as_bytes<char>(obj.dump(4)), ec)) {
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
  if (auto jv = parse_json_file(linkMeta, ec); jv) {
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
        auto le = bela::make_error_code_from_std(e);
        baulk::DbgPrint(L"baulk remove link %s error: %s\n", file, le.message);
      }
    }
    obj["updated"] = bela::FormatTime<char>(bela::Now());
    obj["links"] = newlinks;
    if (!bela::io::AtomicWriteText(linkMeta, bela::io::as_bytes<char>(obj.dump(4)), ec)) {
      return false;
    }
  } catch (const std::exception &e) {
    ec = bela::make_error_code(bela::ErrGeneral, bela::encode_into<char, wchar_t>(e.what()));
    return false;
  }
  return true;
}

class Builder {
public:
  Builder() = default;
  Builder(const Builder &) = delete;
  Builder &operator=(const Builder &) = delete;
  ~Builder() {
    if (!buildPath.empty() && !baulk::IsTraceMode) {
      std::error_code ec;
      std::filesystem::remove_all(buildPath, ec);
    }
  }
  bool Initialize(bela::error_code &ec);
  bool Compile(const baulk::Package &pkg, std::wstring_view source, std::wstring_view appLinks,
               const baulk::LinkMeta &linkMeta, bela::error_code &ec);
  [[nodiscard]] const std::vector<LinkMeta> &LinkMetas() const { return linkmetas; }

private:
  std::filesystem::path buildPath;
  std::vector<LinkMeta> linkmetas;
};

bool Builder::Initialize(bela::error_code &ec) {
  if (auto newTempPath = baulk::fs::NewTempFolder(ec); newTempPath) {
    buildPath = std::move(*newTempPath);
    DbgPrint(L"build root: %v", buildPath.native());
    return true;
  }
  return false;
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

inline void string_overwrite(std::wstring &s, std::wstring_view v) {
  if (!v.empty()) {
    s = v;
  }
}
/*
✅ 🈯️ 💹 ❇️ ✳️ ❎ 🌐 💠 Ⓜ️ 🌀 💤 🏧
🚾 ♿️ 🅿️ 🈳 🈂️ 🛂 🛃 🛄 🛅 🚹 🚺 🚼 🚻 🚮 🎦 📶 🈁 🔣 ℹ️ 🔤 🔡 🔠 🆖 🆗 🆙
🆒 🆕 🆓 0️⃣ 1️⃣ 2️⃣ 3️⃣ 4️⃣ 5️⃣ 6️⃣ 7️⃣ 8️⃣ 9️⃣ 🔟 🔢 #️⃣ *️⃣
⏏️
▶️ ⏸ ⏯ ⏹ ⏺ ⏭ ⏮ ⏩ ⏪ ⏫ ⏬ ◀️ 🔼 🔽 ➡️ ⬅️ ⬆️ ⬇️ ↗️ ↘️ ↙️ ↖️ ↕️ ↔️ ↪️ ↩️ ⤴️ ⤵️ 🔀
🔁 🔂 🔄 🔃 🎵 🎶 ➕ ➖ ➗ ✖️ ♾ 💲 💱 ™️ ©️ ®️ 〰️ ➰ ➿ 🔚 🔙 🔛 🔝 🔜
✔️ ☑️
*/
bool Builder::Compile(const baulk::Package &pkg, std::wstring_view source, std::wstring_view appLinks,
                      const baulk::LinkMeta &linkMeta, bela::error_code &ec) {
  auto realExePath = bela::RealPathEx(source, ec);
  if (!realExePath) {
    return false;
  }
  auto isConsoleExe = bela::pe::IsSubsystemConsole(*realExePath);
  DbgPrint(L"executable %s is subsystem console: %v\n", *realExePath, isConsoleExe);
  auto name = StripExtension(linkMeta.alias);
  auto cxxSourceName = bela::StringCat(name, L".cc");
  auto cxxSourcePath = buildPath / cxxSourceName;
  auto rcSourceName = bela::StringCat(name, L".rc");
  auto rcSourcePath = buildPath / rcSourceName;
  if (!generated::MakeSource(source, cxxSourcePath.native(), isConsoleExe, ec)) {
    return false;
  }
  auto now = bela::LocalDateTime(bela::Now());
  bela::pe::Version version{
      .CompanyName = bela::StringCat(pkg.name, L" contributors"),
      .FileDescription = pkg.description,
      .FileVersion = pkg.version,
      .InternalName = linkMeta.alias,
      .LegalCopyright = bela::StringCat(L"Copyright \xA9 ", now.Year(), L", Baulk contributors 💞."),
      .OriginalFileName = linkMeta.alias,
      .ProductName = pkg.name,
      .ProductVersion = pkg.version,
  };
  if (auto vi = bela::pe::Lookup(source, ec); vi) {
    string_overwrite(version.CompanyName, vi->CompanyName);
    string_overwrite(version.FileDescription, vi->FileDescription);
    string_overwrite(version.FileVersion, vi->FileVersion);
    string_overwrite(version.InternalName, vi->InternalName);
    string_overwrite(version.LegalCopyright, vi->LegalCopyright);
    string_overwrite(version.OriginalFileName, vi->OriginalFileName);
    string_overwrite(version.ProductVersion, vi->ProductVersion);
    string_overwrite(version.ProductName, vi->ProductName);
    string_overwrite(version.Comments, vi->Comments);
    string_overwrite(version.LegalTrademarks, vi->LegalTrademarks);
    string_overwrite(version.PrivateBuild, vi->PrivateBuild);
    string_overwrite(version.SpecialBuild, vi->SpecialBuild);
  }
  DbgPrint(L"compile %s [%s]", cxxSourceName, buildPath.native());
  if (LinkExecutor().Execute(buildPath.native(), L"cl", L"-c", L"-std:c++20", L"-nologo", L"-Os", cxxSourceName) != 0) {
    ec = LinkExecutor().LastErrorCode();
    return false;
  }
  auto subIndex = isConsoleExe ? 0 : 1;
  auto complier_exitcode = [&]() -> int {
    constexpr const std::wstring_view entry[] = {L"-ENTRY:wmain", L"-ENTRY:wWinMain"};
    constexpr const std::wstring_view subsyetmName[] = {L"-SUBSYSTEM:CONSOLE", L"-SUBSYSTEM:WINDOWS"};
    if (generated::MakeResource(version, rcSourcePath.native(), ec)) {
      if (LinkExecutor().Execute(buildPath.native(), L"rc", L"-nologo", L"-c65001", rcSourceName) == 0) {
        return LinkExecutor().Execute(buildPath.native(), L"link", L"-nologo", L"-OPT:REF", L"-OPT:ICF",
                                      L"-NODEFAULTLIB", subsyetmName[subIndex], entry[subIndex],
                                      bela::StringCat(name, L".obj"), bela::StringCat(name, L".res"), L"kernel32.lib",
                                      L"user32.lib", bela::StringCat(L"-OUT:", linkMeta.alias));
      }
    }
    return LinkExecutor().Execute(buildPath.native(), L"link", L"-nologo", L"-OPT:REF", L"-OPT:ICF", L"-NODEFAULTLIB",
                                  subsyetmName[subIndex], entry[subIndex], bela::StringCat(name, L".obj"),
                                  L"kernel32.lib", L"user32.lib", bela::StringCat(L"-OUT:", linkMeta.alias));
  }();

  if (complier_exitcode != 0) {
    ec = LinkExecutor().LastErrorCode();
    return false;
  }
  auto target = bela::StringCat(appLinks, L"\\", linkMeta.alias);
  auto genTarget = buildPath / linkMeta.alias;
  std::error_code e;
  if (std::filesystem::exists(target, e)) {
    std::filesystem::remove_all(target, e);
  }
  std::filesystem::rename(genTarget, target, e);
  if (e) {
    ec = bela::make_error_code_from_std(e);
    return false;
  }
  linkmetas.emplace_back(linkMeta);
  return true;
}

std::optional<std::wstring> path_reachable_cat(const std::filesystem::path &packageRoot, std::wstring_view relativePath,
                                               std::wstring &newRelativePath) {
  std::filesystem::path relativePath_(relativePath);
  auto linkPath = packageRoot / relativePath_;
  std::error_code e;
  if (std::filesystem::exists(linkPath, e)) {
    return std::make_optional(linkPath.wstring());
  }
  std::vector<std::filesystem::path> items;
  for (auto it = relativePath_.begin(); it != relativePath_.end(); it++) {
    items.emplace_back(*it);
  }
  auto make_reachable_path = [&](const std::span<std::filesystem::path> pv) -> std::filesystem::path {
    std::filesystem::path path(packageRoot);
    newRelativePath.clear();
    for (auto p : pv) {
      path /= p;
      if (!newRelativePath.empty()) {
        newRelativePath.append(L"\\");
      }
      newRelativePath.append(p.c_str());
    }
    return path;
  };
  for (size_t i = 1; i < items.size(); i++) {
    std::span itemSpan(items.data() + i, items.size() - i);
    if (itemSpan.empty()) {
      break;
    }
    auto newLinkPath = make_reachable_path(itemSpan);
    if (std::filesystem::exists(newLinkPath, e)) {
      return std::make_optional(newLinkPath.wstring());
    }
  }
  return std::nullopt;
}

bool MakeLaunchers(const baulk::Package &pkg, bool forceoverwrite, bela::error_code &ec) {
  (void)forceoverwrite;
  auto packageRoot = std::filesystem::path(vfs::AppPackageFolder(pkg.name));
  auto appLinks = vfs::AppLinks();
  if (!baulk::fs::MakeDirectories(appLinks, ec)) {
    return false;
  }
  Builder builder;
  if (!builder.Initialize(ec)) {
    return false;
  }
  for (const auto &lm : pkg.launchers) {
    std::wstring relativePath(lm.path);
    auto source = path_reachable_cat(packageRoot, lm.path, relativePath);
    if (!source) {
      bela::FPrintF(stderr, L"unable create launcher '%s': \x1b[31m%s\x1b[0m\n", lm.path, ec);
      continue;
    }
    bela::FPrintF(stderr, L"new launcher: \x1b[35m%v\x1b[0m@\x1b[36m%v\x1b[0m\n", pkg.name, relativePath);
    if (!builder.Compile(pkg, *source, appLinks, lm, ec)) {
      bela::FPrintF(stderr, L"unable create launcher '%s': \x1b[31m%s\x1b[0m\n", lm.path, ec);
    }
  }
  if (!LinkMetaStore(builder.LinkMetas(), pkg, ec)) {
    bela::FPrintF(stderr, L"%s create links error: %s\nYour can run 'baulk uninstall' and retry\n", pkg.name, ec);
    return false;
  }
  return true;
}

std::optional<std::wstring> FindProxyLauncher(bela::error_code &ec) {
  auto proxyLauncher = vfs::AppLocationPath(L"baulk-lnk.exe");
  if (!bela::PathExists(proxyLauncher)) {
    ec = bela::make_error_code(bela::ErrGeneral, L"baulk-lnk.exe not found");
    return std::nullopt;
  }
  if (!vfs::IsPackaged()) {
    return std::make_optional(std::move(proxyLauncher));
  }
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
  auto packageRoot = std::filesystem::path(vfs::AppPackageFolder(pkg.name));
  auto appLinks = vfs::AppLinks();
  if (!baulk::fs::MakeDirectories(appLinks, ec)) {
    return false;
  }
  std::vector<LinkMeta> linkmetas;
  for (const auto &lm : pkg.launchers) {
    std::wstring relativePath(lm.path);
    auto source = path_reachable_cat(packageRoot, lm.path, relativePath);
    if (!source) {
      bela::FPrintF(stderr, L"unable proxy link '%s': \x1b[31m%s\x1b[0m\n", lm.path, ec);
      continue;
    }
    bela::FPrintF(stderr, L"new proxy link: \x1b[35m%v\x1b[0m@\x1b[36m%v\x1b[0m\n", pkg.name, relativePath);
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
    linkmetas.emplace_back(relativePath, lm.alias);
  }
  if (!LinkMetaStore(linkmetas, pkg, ec)) {
    bela::FPrintF(stderr, L"%s create links error: %s\nYour can run 'baulk uninstall' and retry\n", pkg.name, ec);
    return false;
  }
  return true;
}

// create symlink
bool MakeSymlinks(const baulk::Package &pkg, bool forceoverwrite, bela::error_code &ec) {
  auto packageRoot = std::filesystem::path(vfs::AppPackageFolder(pkg.name));
  auto appLinks = vfs::AppLinks();
  if (!baulk::fs::MakeDirectories(appLinks, ec)) {
    return false;
  }
  std::vector<LinkMeta> linkmetas;
  for (auto &lm : pkg.links) {
    std::wstring relativePath(lm.path);
    auto source = path_reachable_cat(packageRoot, lm.path, relativePath);
    if (!source) {
      bela::FPrintF(stderr, L"unable create link '%s': \x1b[31m%s\x1b[0m\n", lm.path, ec);
      continue;
    }
    bela::FPrintF(stderr, L"new link: \x1b[35m%v\x1b[0m@\x1b[36m%v\x1b[0m\n", pkg.name, relativePath);
    auto lnk = bela::StringCat(appLinks, L"\\", lm.alias);
    if (bela::PathExists(lnk)) {
      if (forceoverwrite) {
        bela::fs::ForceDeleteFolders(lnk, ec);
      }
    }
    if (!baulk::fs::SymLink(*source, lnk, ec)) {
      DbgPrint(L"make %s -> %s: %s\n", *source, lnk, ec);
      return false;
    }
    linkmetas.emplace_back(relativePath, lm.alias);
  }
  if (!LinkMetaStore(linkmetas, pkg, ec)) {
    bela::FPrintF(stderr, L"%s create links error: %s\nYour can run 'baulk uninstall' and retry\n", pkg.name, ec);
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
