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

inline std::optional<std::wstring> ResolveTarget(bela::error_code &ec) {
  auto arg0 = bela::Executable(ec); // do not resolve symlink !
  if (!arg0) {
    return std::nullopt;
  }
  baulk::DbgPrint(L"resolve arg0: %v", *arg0);
  std::filesystem::path fsArg0(*arg0);
  auto command = fsArg0.filename();
  baulk::DbgPrint(L"resolve launcher: %v", command);
  auto linkMeta = fsArg0.parent_path() / L"baulk.linkmeta.json";
  baulk::DbgPrint(L"resolve link metadata: %v", linkMeta);
  auto jo = baulk::parse_json_file(linkMeta.native(), ec);
  if (!jo) {
    baulk::DbgPrint(L"resolve link metadata error: %v", ec);
    return std::nullopt;
  }
  auto jv = jo->view();
  auto sv = jv.subview("links");
  if (!sv) {
    ec = bela::make_error_code(bela::ErrGeneral, L"link metadata no links");
    return std::nullopt;
  }
  auto linkTarget = sv->fetch(bela::encode_into<wchar_t, char>(command.native()));
  std::vector<std::wstring_view> tv = bela::StrSplit(linkTarget, bela::ByChar('@'), bela::SkipEmpty());
  if (tv.size() < 2) {
    ec = bela::make_error_code(bela::ErrGeneral, L"'", command, L"' not found. (", linkTarget, L") path:", *arg0);
    return std::nullopt;
  }
  auto appPackagePath = jv.fetch("app_packages_root");
  return std::make_optional<>(bela::StringCat(appPackagePath, L"\\", tv[0], L"\\", tv[1]));
}

#endif