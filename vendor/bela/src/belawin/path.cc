////
#include <vector>
#include <bela/path.hpp>
#include <bela/strip.hpp>
#include <bela/str_cat.hpp>
#include <bela/str_split.hpp>
#include <bela/ascii.hpp>
#include <bela/base.hpp>
#include <bela/env.hpp>

namespace bela {

namespace path_internal {
template <typename T> class Literal;
template <> class Literal<char> {
public:
  static constexpr std::string_view Dot = ".";
  static constexpr std::string_view DotDot = "..";
  static constexpr std::string_view PathSeparators = "\\/";
};
template <> class Literal<wchar_t> {
public:
  static constexpr std::wstring_view Dot = L".";
  static constexpr std::wstring_view DotDot = L"..";
  static constexpr std::wstring_view PathSeparators = L"\\/";
};
template <> class Literal<char16_t> {
public:
  static constexpr std::u16string_view Dot = u".";
  static constexpr std::u16string_view DotDot = u"..";
  static constexpr std::u16string_view PathSeparators = u"\\/";
};
template <> class Literal<char8_t> {
public:
  static constexpr std::u8string_view Dot = u8".";
  static constexpr std::u8string_view DotDot = u8"..";
  static constexpr std::u8string_view PathSeparators = u8"\\/";
};

} // namespace path_internal

// https://docs.microsoft.com/en-us/windows/desktop/FileIO/naming-a-file
// https://googleprojectzero.blogspot.com/2016/02/the-definitive-guide-on-win32-to-nt.html
bool IsReservedName(std::wstring_view name) {
  constexpr std::wstring_view reserved_names[] = {
      L"CON",  L"PRN",  L"AUX",  L"NUL",  L"COM1", L"COM2", L"COM3", L"COM4", L"COM5", L"COM6", L"COM7",
      L"COM8", L"COM9", L"LPT1", L"LPT2", L"LPT3", L"LPT4", L"LPT5", L"LPT6", L"LPT7", L"LPT8", L"LPT9"};
  return std::find(std::begin(reserved_names), std::end(reserved_names), bela::AsciiStrToUpper(name)) ==
         std::end(reserved_names);
}

inline std::wstring FullPathInternal(std::wstring_view p) {
  std::wstring absPath;
  DWORD absLen = MAX_PATH;
  for (;;) {
    absPath.resize(absLen);
    if (absLen = GetFullPathNameW(p.data(), absLen, absPath.data(), nullptr); absLen == 0) {
      return L"";
    }
    if (absLen < absPath.size()) {
      absPath.resize(absLen);
      break;
    }
  }
  return absPath;
}

std::wstring FullPath(std::wstring_view p) {
  if (p.starts_with('~')) {
    return FullPathInternal(bela::StringCat(bela::GetEnv(L"USERPROFILE"), L"//", p.substr(1)));
  }
  return FullPathInternal(p);
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
  if (path.empty()) {
    return L".";
  }
  auto i = path.size() - 1;
  auto p = path.data();
  for (; IsPathSeparator(p[i]); i--) {
    if (i == 0) {
      return L"/";
    }
  }
  for (; !IsPathSeparator(p[i]); i--) {
    if (i == 0) {
      return L".";
    }
  }
  for (; IsPathSeparator(p[i]); i--) {
    if (i == 0) {
      return L"/";
    }
  }
  return path.substr(0, i + 1);
}

template <typename C>
bool SplitPathInternal(std::basic_string_view<C, std::char_traits<C>> sv,
                       std::vector<std::basic_string_view<C, std::char_traits<C>>> &output) {
  size_t first = 0;
  while (first < sv.size()) {
    const auto second = sv.find_first_of(path_internal::Literal<C>::PathSeparators, first);
    if (first != second) {
      auto s = sv.substr(first, second - first);
      if (s == path_internal::Literal<C>::DotDot) {
        if (output.empty()) {
          return false;
        }
        output.pop_back();
      } else if (s != path_internal::Literal<C>::Dot) {
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

std::vector<std::string_view> SplitPath(std::string_view sv) {
  std::vector<std::string_view> pv;
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
  if (p.size() >= 4 && bela::IsPathSeparator(p[0]) && bela::IsPathSeparator(p[1]) && !bela::IsPathSeparator(p[2]) &&
      (p[2] == '.' || p[2] == '?') && bela::IsPathSeparator(p[3])) {
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

std::wstring parth_cat_pieces(std::wstring_view p0, std::span<std::wstring_view> pieces) {
  auto root = PathStripRootName(p0);
  std::vector<std::wstring_view> pv;
  SplitPathInternal(p0, pv);
  for (const auto p : pieces) {
    if (!SplitPathInternal(p, pv)) {
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

std::wstring PathCatPieces(std::span<std::wstring_view> pieces) {
  if (pieces.empty()) {
    return L"";
  }
  return parth_cat_pieces(pieces[0], pieces.subspan(1));
}

std::wstring JoinPathPieces(std::span<std::wstring_view> pieces) {
  if (pieces.empty()) {
    return L"";
  }
  return parth_cat_pieces(bela::FullPath(pieces[0]), pieces.subspan(1));
}

} // namespace path_internal
} // namespace bela
