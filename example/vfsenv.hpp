///
#ifndef VFSENV_HPP
#define VFSENV_HPP
#include <bela/base.hpp>
#include <bela/path.hpp>

namespace example {
inline bool PathFileIsExists(std::wstring_view file) {
  auto at = GetFileAttributesW(file.data());
  return (INVALID_FILE_ATTRIBUTES != at && (at & FILE_ATTRIBUTE_DIRECTORY) == 0);
}

class PathSearcher {
public:
  PathSearcher(const PathSearcher &) = delete;
  PathSearcher &operator=(const PathSearcher &) = delete;
  static PathSearcher &Instance() {
    static PathSearcher inst;
    return inst;
  }
  std::wstring JoinEtc(std::wstring_view p) { return bela::StringCat(etc, L"\\", p); }
  std::wstring JoinPath(std::wstring_view p) { return bela::StringCat(root, L"\\", p); }

private:
  std::wstring etc{L"."};
  std::wstring root{L"."};
  PathSearcher() {
    bela::error_code ec;
    auto bk = SearchBaulkRoot(ec);
    if (bk) {
      root.assign(std::move(*bk));
      etc = bela::StringCat(root, L"\\bin\\etc");
      return;
    }
    auto exeparent = bela::ExecutableFinalPathParent(ec);
    if (exeparent) {
      root.assign(std::move(*exeparent));
      etc = root;
    }
  }
  std::optional<std::wstring> SearchBaulkRoot(bela::error_code &ec) {
    auto exepath = bela::ExecutableParent(ec);
    if (!exepath) {
      return std::nullopt;
    }
    std::wstring_view baulkroot(*exepath);
    for (size_t i = 0; i < 5; i++) {
      if (baulkroot == L".") {
        break;
      }
      auto baulkexe = bela::StringCat(baulkroot, L"\\bin\\baulk.exe");
      if (PathFileIsExists(baulkexe)) {
        return std::make_optional<std::wstring>(baulkroot);
      }
      baulkroot = bela::DirName(baulkroot);
    }
    ec = bela::make_error_code(1, L"unable found baulk.exe");
    return std::nullopt;
  }
};

} // namespace example

#endif