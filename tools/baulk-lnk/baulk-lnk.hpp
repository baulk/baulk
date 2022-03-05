///
#ifndef BAULK_LNK_HPP
#define BAULK_LNK_HPP
#include <bela/base.hpp>
#include <bela/escapeargv.hpp>
#include <bela/pe.hpp>
#include <bela/terminal.hpp>
#include <bela/path.hpp>
#include <bela/str_split.hpp>
#include <bela/env.hpp>
#include <version.hpp>
#include <filesystem>
#include <baulk/json_utils.hpp>
#include <baulk/debug.hpp>

namespace fs = std::filesystem;

inline bool IsSubsytemConsole(std::wstring_view exe) {
  bela::error_code ec;
  auto realexe = bela::RealPathEx(exe, ec);
  if (!realexe) {
    return false;
  }
  return bela::pe::IsSubsystemConsole(*realexe);
}

inline bool IsTrue(std::wstring_view b) {
  return bela::EqualsIgnoreCase(b, L"true") || bela::EqualsIgnoreCase(b, L"yes") || b == L"1";
}

constexpr std::wstring_view linkMetaName = L"\\baulk.linkmeta.json";

inline std::optional<std::wstring> ResolveTarget(bela::error_code &ec) {
  auto lnkFullName = bela::Executable(ec); // do not resolve symlink !
  if (!lnkFullName) {
    return std::nullopt;
  }
  auto lnkBaseName = bela::BaseName(*lnkFullName);
  auto lnkDirName = bela::DirName(*lnkFullName);
  auto linkMetaFullName = bela::StringCat(lnkDirName, linkMetaName);
  auto jo = baulk::json::parse_file(bela::StringCat(lnkDirName, linkMetaName), ec);
  if (!jo) {
    return std::nullopt;
  }
  auto jv = jo->view().subview("links");
  if (!jv) {
    ec = bela::make_error_code(bela::ErrGeneral, L"baulk launcher cannot find the command '", lnkBaseName,
                               L"'. meta no links");
    return std::nullopt;
  }
  auto linkTarget = jv->fetch(bela::encode_into<wchar_t, char>(lnkBaseName));
  std::vector<std::wstring_view> tv = bela::StrSplit(linkTarget, bela::ByChar('@'), bela::SkipEmpty());
  if (tv.size() < 2) {
    ec = bela::make_error_code(bela::ErrGeneral, L"baulk launcher cannot find the command '", lnkBaseName,
                               L"'. target --> [", linkTarget, L"] lnkName:", *lnkFullName);
    return std::nullopt;
  }
  auto appPackagePath = jv->fetch("app_packages_root", bela::StringCat(bela::DirName(lnkDirName), L"\\pkgs"));
  return std::make_optional<>(bela::StringCat(appPackagePath, L"\\", tv[0], L"\\", tv[1]));
}

#endif