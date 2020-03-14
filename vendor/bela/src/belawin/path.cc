////
#include <vector>
#include <bela/path.hpp>
#include <bela/strip.hpp>
#include <bela/strcat.hpp>
#include <bela/str_split.hpp>
#include <bela/ascii.hpp>
#include <bela/base.hpp>
#include <bela/env.hpp>

namespace bela {
// https://docs.microsoft.com/en-us/windows/desktop/FileIO/naming-a-file
// https://googleprojectzero.blogspot.com/2016/02/the-definitive-guide-on-win32-to-nt.html
inline bool PathStripExtended(std::wstring_view &sv, wchar_t &ch) {
  if (sv.size() < 4) {
    return false;
  }
  if (sv[0] == '\\' && sv[1] == '\\' && (sv[2] == '?' || sv[2] == '.') &&
      sv[3] == '\\') {
    ch = sv[2];
    sv.remove_prefix(4);
    return true;
  }
  return false;
}

inline bool PathStripDriveLatter(std::wstring_view &sv, wchar_t &dl) {
  if (sv.size() < 2) {
    return false;
  }
  if (sv[1] == L':') {
    auto ch = sv[0];
    if (ch > 'a' && ch <= 'z') {
      dl = tolower(ch);
      sv.remove_prefix(2);
      return true;
    }
    if (ch >= 'A' && ch <= 'Z') {
      dl = ch;
      sv.remove_prefix(2);
      return true;
    }
  }

  return false;
}

bool SplitPathInternal(std::wstring_view sv,
                       std::vector<std::wstring_view> &output) {
  constexpr std::wstring_view dotdot = L"..";
  constexpr std::wstring_view dot = L".";
  size_t first = 0;
  while (first < sv.size()) {
    const auto second = sv.find_first_of(L"/\\", first);
    if (first != second) {
      auto s = sv.substr(first, second - first);
      if (s == dotdot) {
        if (output.empty()) {
          return false;
        }
        output.pop_back();
      } else if (s != dot) {
        output.emplace_back(s);
      }
    }
    if (second == std::wstring_view::npos) {
      break;
    }
    first = second + 1;
  }
  return true;
}

std::vector<std::wstring_view> SplitPath(std::wstring_view sv) {
  std::vector<std::wstring_view> pv;
  if (!SplitPathInternal(sv, pv)) {
    pv.clear();
  }
  return pv;
}

void PathStripName(std::wstring &s) {
  auto i = s.size() - 1;
  auto p = s.data();
  for (; IsPathSeparator(p[i]); i--) {
    if (i == 0) {
      return;
    }
  }
  for (; !IsPathSeparator(p[i]); i--) {
    if (i == 0) {
      return;
    }
  }
  for (; IsPathSeparator(p[i]); i--) {
    if (i == 0) {
      return;
    }
  }
  s.resize(i + 1);
}

namespace path_internal {

std::wstring getcurrentdir() {
  auto l = GetCurrentDirectoryW(0, nullptr);
  if (l == 0) {
    return L"";
  }
  std::wstring s;
  s.resize(l);
  auto N = GetCurrentDirectoryW(l, s.data());
  s.resize(N);
  return s;
}

std::wstring PathJoinInternal(std::vector<std::wstring_view> &pv, bool unc,
                              bool full, wchar_t uncchar, wchar_t latter) {
  std::wstring s;
  size_t alsize = unc ? 4 : 0;
  if (full) {
    alsize += 2;
  }
  for (const auto p : pv) {
    alsize += p.size() + 1;
  }
  s.reserve(alsize);
  if (unc) {
    s.append(L"\\\\").push_back(uncchar);
    s.push_back(L'\\');
  }
  if (full) {
    s.push_back(latter);
    s.push_back(L':');
  }
  for (const auto p : pv) {
    if (!s.empty()) {
      s.push_back(L'\\');
    }
    s.append(p);
  }
  return s;
}

std::wstring PathCatPieces(bela::Span<std::wstring_view> pieces) {
  if (pieces.empty()) {
    return L"";
  }
  auto p0 = pieces[0];
  std::wstring p0raw;
  if (p0 == L".") {
    p0raw = getcurrentdir();
    p0 = p0raw;
  }
  wchar_t exch = 0;
  auto isextend = PathStripExtended(p0, exch);
  wchar_t latter = 0;
  auto haslatter = PathStripDriveLatter(p0, latter);
  std::vector<std::wstring_view> pv;
  if (!SplitPathInternal(p0, pv)) {
    return L"";
  }
  for (size_t i = 1; i < pieces.size(); i++) {
    if (!SplitPathInternal(pieces[i], pv)) {
      return L"";
    }
  }
  return PathJoinInternal(pv, isextend, haslatter, exch, latter);
}

} // namespace path_internal

std::wstring PathAbsolute(std::wstring_view p) {
  if (p.empty()) {
    return L".";
  }
  wchar_t exch = 0;
  auto isextend = PathStripExtended(p, exch);
  wchar_t latter = 0;
  auto haslatter = PathStripDriveLatter(p, latter);
  std::vector<std::wstring_view> pv;
  std::wstring cwd;
  if (!isextend && !haslatter) {
    cwd = path_internal::getcurrentdir();
    SplitPathInternal(cwd, pv);
  }
  if (!SplitPathInternal(p, pv)) {
    return L"";
  }
  return path_internal::PathJoinInternal(pv, isextend, haslatter, exch, latter);
}

bool PathExists(std::wstring_view src, FileAttribute fa) {
  // GetFileAttributesExW()
  auto a = GetFileAttributesW(src.data());
  if (a == INVALID_FILE_ATTRIBUTES) {
    return false;
  }
  return fa == FileAttribute::None ? true : ((static_cast<DWORD>(fa) & a) != 0);
}

inline bool PathFileIsExists(std::wstring_view file) {
  auto at = GetFileAttributesW(file.data());
  return (INVALID_FILE_ATTRIBUTES != at &&
          (at & FILE_ATTRIBUTE_DIRECTORY) == 0);
}

bool HasExt(std::wstring_view file) {
  auto pos = file.find(L'.');
  if (pos == std::wstring_view::npos) {
    return false;
  }
  return (file.find_last_of(L":\\/") < pos);
}

bool FindExecutable(std::wstring_view file,
                    const std::vector<std::wstring> &exts, std::wstring &p) {
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

bool ExecutableExistsInPath(std::wstring_view cmd, std::wstring &exe) {
  constexpr std::wstring_view defaultexts[] = {L".com", L".exe", L".bat",
                                               L".cmd"};
  std::wstring suffixwapper;
  std::vector<std::wstring> exts;
  auto pathext = GetEnv<64>(L"PATHEXT");
  if (!pathext.empty()) {
    bela::AsciiStrToLower(&pathext); // tolower
    exts = bela::StrSplit(pathext, bela::ByChar(L';'), bela::SkipEmpty());
  } else {
    exts.assign(std::begin(defaultexts), std::end(defaultexts));
  }
  if (cmd.find_first_of(L":\\/") != std::wstring_view::npos) {
    auto ncmd = bela::PathAbsolute(cmd);
    return FindExecutable(ncmd, exts, exe);
  }
  auto cwdfile = bela::PathCat(L".", cmd);
  if (FindExecutable(cwdfile, exts, exe)) {
    return true;
  }
  auto path = GetEnv<4096>(L"PATH"); // 4K suggest.
  std::vector<std::wstring_view> pathv =
      bela::StrSplit(path, bela::ByChar(L';'), bela::SkipEmpty());
  for (auto p : pathv) {
    auto pfile = bela::PathCat(p, cmd);
    if (FindExecutable(pfile, exts, exe)) {
      return true;
    }
  }
  return false;
}

std::optional<std::wstring> Executable(bela::error_code &ec) {
  std::wstring buffer;
  buffer.resize(MAX_PATH);
  auto X = GetModuleFileNameW(nullptr, buffer.data(), MAX_PATH);
  if (X == 0) {
    ec = bela::make_system_error_code();
    return std::nullopt;
  }
  if (X <= MAX_PATH) {
    buffer.resize(X);
    return std::make_optional(std::move(buffer));
  }
  buffer.resize(X);
  auto Y = GetModuleFileNameW(nullptr, buffer.data(), X);
  if (Y == 0) {
    ec = bela::make_system_error_code();
    return std::nullopt;
  }
  if (Y <= X) {
    buffer.resize(Y);
    return std::make_optional(std::move(buffer));
  }
  ec = bela::make_error_code(-1, L"Executable unable allocate more buffer");
  return std::nullopt;
}

std::optional<std::wstring> ExecutablePath(bela::error_code &ec) {
  auto exe = Executable(ec);
  if (!exe) {
    return std::nullopt;
  }
  PathStripName(*exe);
  return std::make_optional(std::move(*exe));
}

} // namespace bela
