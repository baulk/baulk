///
#include <vector>
#include <bela/path.hpp>
#include <bela/strip.hpp>
#include <bela/strcat.hpp>
#include <bela/str_split.hpp>
#include <bela/ascii.hpp>
#include <bela/base.hpp>
#include <bela/env.hpp>

namespace bela {

inline bool HasExt(std::wstring_view file) {
  if (auto pos = file.rfind(L'.'); pos == std::wstring_view::npos) {
    return file.find_last_of(L":\\/") < pos;
  }
  return false;
}

bool FindExecutable(std::wstring_view file, const std::vector<std::wstring> &exts,
                    std::wstring &p) {
  if (HasExt(file) && PathFileIsExists(file)) {
    p = file;
    return true;
  }
  std::wstring newfile;
  newfile.reserve(file.size() + 8);
  newfile.assign(file);
  auto rawsize = newfile.size();
  for (const auto &e : exts) {
    // rawsize always < newfile.size();
    // std::char_traits::assign
    newfile.resize(rawsize);
    newfile.append(e);
    if (PathFileIsExists(newfile)) {
      p.assign(std::move(newfile));
      return true;
    }
  }
  return false;
}

inline void InitializeExtensions(std::vector<std::wstring> &exts) {
  constexpr std::wstring_view defaultexts[] = {L".com", L".exe", L".bat", L".cmd"};
  auto pathext = GetEnv<64>(L"PATHEXT");
  if (!pathext.empty()) {
    bela::AsciiStrToLower(&pathext); // tolower
    exts = bela::StrSplit(pathext, bela::ByChar(L';'), bela::SkipEmpty());
    return;
  }
  exts.assign(std::begin(defaultexts), std::end(defaultexts));
}

bool ExecutableExistsInPath(std::wstring_view cmd, std::wstring &exe,
                            const std::vector<std::wstring> &paths) {
  std::vector<std::wstring> exts;
  InitializeExtensions(exts);
  if (cmd.find_first_of(L":\\/") != std::wstring_view::npos) {
    auto ncmd = bela::PathAbsolute(cmd);
    return FindExecutable(ncmd, exts, exe);
  }
  auto cwdfile = bela::PathAbsolute(cmd);
  if (FindExecutable(cwdfile, exts, exe)) {
    return true;
  }
  for (auto p : paths) {
    auto exefile = bela::StringCat(p, L"\\", cmd);
    if (FindExecutable(exefile, exts, exe)) {
      return true;
    }
  }
  return false;
}

bool ExecutableExistsInPath(std::wstring_view cmd, std::wstring &exe) {
  std::vector<std::wstring> exts;
  InitializeExtensions(exts);
  if (cmd.find_first_of(L":\\/") != std::wstring_view::npos) {
    auto ncmd = bela::PathAbsolute(cmd);
    return FindExecutable(ncmd, exts, exe);
  }
  auto cwdfile = bela::PathAbsolute(cmd);
  if (FindExecutable(cwdfile, exts, exe)) {
    return true;
  }
  auto path = GetEnv<4096>(L"PATH"); // 4K suggest.
  std::vector<std::wstring_view> pathv =
      bela::StrSplit(path, bela::ByChar(L';'), bela::SkipEmpty());
  for (auto p : pathv) {
    auto exefile = bela::StringCat(p, L"\\", cmd);
    if (FindExecutable(exefile, exts, exe)) {
      return true;
    }
  }
  return false;
}
} // namespace bela