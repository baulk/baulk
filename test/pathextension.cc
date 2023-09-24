///
#include <bela/terminal.hpp>
#include <bela/path.hpp>
#include <bela/str_split_narrow.hpp>
#include <filesystem>

constexpr bool is_dot_or_separator(wchar_t ch) { return bela::IsPathSeparator(ch) || ch == L'.'; }

std::wstring_view PathStripExtension(std::wstring_view p) {
  if (p.empty()) {
    return L".";
  }
  auto i = p.size() - 1;
  for (; i != 0 && is_dot_or_separator(p[i]); i--) {
    p.remove_suffix(1);
  }
  constexpr std::wstring_view extensions[] = {L".tar.gz",   L".tar.bz2", L".tar.xz", L".tar.zst",
                                              L".tar.zstd", L".tar.br",  L".tar.lz4"};
  for (const auto e : extensions) {
    if (bela::EndsWithIgnoreCase(p, e)) {
      return p.substr(0, p.size() - e.size());
    }
  }
  if (auto pos = p.find_last_of(L"\\/."); pos != std::wstring_view::npos && p[pos] == L'.') {
    // if (pos >= 1 && bela::IsPathSeparator(p[pos - 1])) {
    //   return p;
    // }
    return p.substr(0, pos);
  }
  return p;
}

inline std::wstring FileDestination(std::wstring_view arfile) {
  auto ends_with_path_separator = [](std::wstring_view p) -> bool {
    if (p.empty()) {
      return false;
    }
    return bela::IsPathSeparator(p.back());
  };
  if (auto d = PathStripExtension(arfile); d.size() != arfile.size() && !ends_with_path_separator(d)) {
    return std::wstring(d);
  }
  return bela::StringCat(arfile, L".out");
}

void pathfstest() {
  std::filesystem::path root(L"C:\\Baulk");
  const std::filesystem::path childs[] = {
      L"C:\\Baulk\\bin\\baulk.exe",        //
      L"C:\\Baulk\\bin\\baulk-exec.exe",   //
      L"C:\\Baulk\\bin\\baulk-update.exe", //
      L"C:\\Baulk\\config\\baulk.json",    //
      L"C:\\Baulk\\baulk.env",             //
  };
  const std::filesystem::path filter_paths[] = {
      L"bin/baulk-exec.exe",   //
      L"bin/baulk-update.exe", //
  };
  auto is_file_should_rename = [&](const std::filesystem::path &p) -> bool {
    for (const auto &i : filter_paths) {
      if (i == p) {
        return true;
      }
    }
    return false;
  };
  std::error_code e;
  for (const auto &p : childs) {
    auto relativePath = std::filesystem::relative(p, root, e);
    if (e) {
      continue;
    }
    if (is_file_should_rename(relativePath)) {
      bela::FPrintF(stderr, L"rename: %s\n", p.c_str());
      continue;
    }
    bela::FPrintF(stderr, L"apply: %s\n", p.c_str());
  }
}

inline bool is_harmful_path(std::string_view child_path) {
  const std::string_view dot = ".";
  const std::string_view dotdot = "..";
  std::vector<std::string_view> paths =
      bela::narrow::StrSplit(child_path, bela::narrow::ByAnyChar("\\/"), bela::narrow::SkipEmpty());
  int entries = 0;
  for (auto p : paths) {
    if (p == dot) {
      continue;
    }
    if (p != dotdot) {
      entries++;
      continue;
    }
    entries--;
    if (entries < 0) {
      return true;
    }
  }
  return entries <= 0;
}

bool create_symlink(const std::filesystem::path &destination, const std::filesystem::path &_New_symlink,
                    std::string_view linkname, bool always_utf8, bela::error_code &ec) {
  auto nativeLinkName = bela::encode_into<char, wchar_t>(linkname);
  std::error_code e;
  auto linkPath = std::filesystem::absolute(_New_symlink.parent_path() / nativeLinkName, e);
  if (e) {
    ec = bela::make_error_code_from_std(e, L"absolute() ");
    return false;
  }
  auto rel = std::filesystem::relative(linkPath, destination, e);
  if (e) {
    ec = bela::make_error_code_from_std(e, L"relative() ");
    return false;
  }
  bela::FPrintF(stderr, L"%s --R--> %s\n", linkname, rel);
  if (bela::StrContains(rel.c_str(), L"..\\")) {
     bela::FPrintF(stderr, L"%s BAD\n", linkPath, rel);
    ec = bela::make_error_code(bela::ErrGeneral, L"harmful path: ", nativeLinkName);
    return false;
  }
  return true;
}

void createSymlinkMock() {
  constexpr std::string_view svs[] = {
      //
      "../../..",            // bad
      "././jack",            // good
      "../zzz/../..",        // bad
      "./././zz/../..",      // bad
      "zzz/../zz",           // good
      "zzz/../../../zzz",    // bad
      "abc/file.zip"         // good
      "ac/././././file.zip", // good
      "../../../z",          // bad
      "../LICENSE",          // GOOD
  };
  std::filesystem::path dest = L"C:\\Baulk\\Out";
  std::filesystem::path newlink = L"C:\\Baulk\\Out\\python\\LICENSE";
  for (const auto s : svs) {
    bela::error_code ec;
    if (!create_symlink(dest, newlink, s, true, ec)) {
      bela::FPrintF(stderr, L"\x1b[31m%s\x1b[0m --> E: %s\n", s, ec);
      continue;
    }
    bela::FPrintF(stderr, L"%s OK\n", s);
  }
}

int wmain() {
  constexpr std::string_view svs[] = {
      //
      "../",                       // bad
      "././jack",                  // good
      "../zzz",                    // bad
      "./././zz/..",               // bad
      "zzz/../zz",                 // good
      "zzz/../../zzz",             // bad
      "abc/file.zip"               // good
      "ac/././././file.zip",       // good
      "../../",                    // bad
      "python/LICENSE/../LICENSE", // GOOD
  };
  for (const auto s : svs) {
    bela::FPrintF(stderr, L"[%v] is harmful path: %v\n", s, is_harmful_path(s));
  }

  constexpr std::wstring_view paths[] = {L"",
                                         L"//////////////////",
                                         L"/home/path/some.tgz",
                                         L"/home/path/folder",
                                         L"/home/path/some.tgz//////////////",
                                         L"/home/path/some.tgz.tar.gz",
                                         L"C:/jackson.zip",
                                         L"/home/path/some.tgz....",
                                         L"/home/path/.zip",
                                         L"/home/path/abc.exe",
                                         L"./zzz.zip"};
  for (const auto sv : paths) {
    bela::FPrintF(stderr, L"[%s]--> %s | %s\n", sv, PathStripExtension(sv), FileDestination(sv));
  }
  pathfstest();
  createSymlinkMock();
  return 0;
}