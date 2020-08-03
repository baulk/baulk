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
bool IsReservedName(std::wstring_view name) {
  constexpr std::wstring_view reservednames[] = {
      L"CON",  L"PRN",  L"AUX",  L"NUL",  L"COM1", L"COM2", L"COM3", L"COM4",
      L"COM5", L"COM6", L"COM7", L"COM8", L"COM9", L"LPT1", L"LPT2", L"LPT3",
      L"LPT4", L"LPT5", L"LPT6", L"LPT7", L"LPT8", L"LPT9"};
  for (const auto r : reservednames) {
    if (bela::EqualsIgnoreCase(r, name)) {
      return true;
    }
  }
  return false;
}

std::wstring PathAbsolute(std::wstring_view p) {
  DWORD dwlen = MAX_PATH;
  std::wstring buf;
  for (;;) {
    buf.resize(dwlen);
    dwlen = GetFullPathNameW(p.data(), dwlen, buf.data(), nullptr);
    if (dwlen == 0) {
      return L"";
    }
    if (dwlen < buf.size()) {
      buf.resize(dwlen);
      break;
    }
  }
  return buf;
}

std::wstring_view BaseName(std::wstring_view name) {
  if (name.empty()) {
    return L".";
  }
  if (name.size() == 2 && name[1] == ':') {
    return L".";
  }
  if (name.size() > 2 && name[1] == ':') {
    name.remove_prefix(2);
  }
  auto i = name.size() - 1;
  for (; i != 0 && bela::IsPathSeparator(name[i]); i--) {
    name.remove_suffix(1);
  }
  if (name.size() == 1 && bela::IsPathSeparator(name[0])) {
    return L".";
  }
  for (; i != 0 && !bela::IsPathSeparator(name[i - 1]); i--) {
  }
  return name.substr(i);
}

std::wstring_view DirName(std::wstring_view path) {
  auto i = path.size() - 1;
  auto p = path.data();
  for (; IsPathSeparator(p[i]); i--) {
    if (i == 0) {
      return path;
    }
  }
  for (; !IsPathSeparator(p[i]); i--) {
    if (i == 0) {
      return path;
    }
  }
  for (; IsPathSeparator(p[i]); i--) {
    if (i == 0) {
      return path;
    }
  }
  return path.substr(0, i + 1);
}

bool SplitPathInternal(std::wstring_view sv, std::vector<std::wstring_view> &output) {
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
size_t PathRootLen(std::wstring_view p) {
  if (p.size() < 2) {
    return 0;
  }
  if (p[1] == ':' && bela::ascii_isalpha(p[0])) {
    return 2;
  }
  if (p.size() >= 4 && bela::IsPathSeparator(p[0]) && bela::IsPathSeparator(p[1]) &&
      !bela::IsPathSeparator(p[2]) && (p[2] == '.' || p[2] == '?') && bela::IsPathSeparator(p[3])) {
    if (p.size() >= 6) {
      if (p[5] == ':' && bela::ascii_isalpha(p[4])) {
        return 6;
      }
      return 4;
    }
  }
  if (auto l = p.size(); l >= 5 && bela::IsPathSeparator(p[0]) && bela::IsPathSeparator(p[1]) &&
                         !bela::IsPathSeparator(p[2]) && p[2] != '.') {
    // first, leading `\\` and next shouldn't be `\`. its server name.
    for (size_t n = 3; n < l - 1; n++) {
      // second, next '\' shouldn't be repeated.
      if (bela::IsPathSeparator(p[n])) {
        n++;
        // third, following something characters. its share name.
        if (!bela::IsPathSeparator(p[n])) {
          if (p[n] == '.') {
            break;
          }
          for (; n < l; n++) {
            if (bela::IsPathSeparator(p[n])) {
              break;
            }
          }
          return n;
        }
        break;
      }
    }
  }
  return 0;
}

std::wstring_view PathStripRootName(std::wstring_view &p) {
  auto rlen = PathRootLen(p);
  if (rlen != 0) {
    auto root = p.substr(0, rlen);
    p.remove_prefix(rlen);
    return root;
  }
  return L"";
}

std::wstring PathAbsoluteCatPieces(bela::Span<std::wstring_view> pieces) {
  if (pieces.empty()) {
    return L"";
  }
  auto p0 = bela::PathAbsolute(pieces[0]);
  std::wstring_view p0s = p0;
  auto root = PathStripRootName(p0s);
  std::vector<std::wstring_view> pv;
  SplitPathInternal(p0s, pv);
  for (size_t i = 1; i < pieces.size(); i++) {
    if (!SplitPathInternal(pieces[i], pv)) {
      return L".";
    }
  }
  std::wstring s;
  auto alsize = root.size();
  for (const auto p : pv) {
    alsize += p.size() + 1;
  }
  s.reserve(alsize + 1);
  s.assign(root);
  for (const auto p : pv) {
    if (!s.empty()) {
      s.push_back(L'\\');
    }
    s.append(p);
  }
  return s;
}

std::wstring PathCatPieces(bela::Span<std::wstring_view> pieces) {
  std::wstring_view p0s = pieces[0];
  auto root = PathStripRootName(p0s);
  std::vector<std::wstring_view> pv;
  SplitPathInternal(p0s, pv);
  for (size_t i = 1; i < pieces.size(); i++) {
    if (!SplitPathInternal(pieces[i], pv)) {
      return L".";
    }
  }
  std::wstring s;
  auto alsize = root.size();
  for (const auto p : pv) {
    alsize += p.size() + 1;
  }
  s.reserve(alsize + 1);
  if (!root.empty()) {
    s.assign(root);
  }
  for (const auto p : pv) {
    if (!s.empty()) {
      s.push_back(L'\\');
    }
    s.append(p);
  }
  return s;
}

} // namespace path_internal
} // namespace bela
