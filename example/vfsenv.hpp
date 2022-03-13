///
#ifndef VFSENV_HPP
#define VFSENV_HPP
#include <bela/base.hpp>
#include <bela/path.hpp>
#include <filesystem>
#include "json.hpp"

namespace example {
inline std::optional<nlohmann::json> parse_file(const std::wstring_view file, bela::error_code &ec) {
  FILE *fd = nullptr;
  if (auto eno = _wfopen_s(&fd, file.data(), L"rb"); eno != 0) {
    ec = bela::make_stdc_error_code(eno, bela::StringCat(L"open json file '", bela::BaseName(file), L"' "));
    return std::nullopt;
  }
  auto closer = bela::finally([&] { fclose(fd); });
  try {
    // comments support
    return std::make_optional(nlohmann::json::parse(fd, nullptr, true, true));
  } catch (const std::exception &e) {
    ec = bela::make_error_code(bela::ErrGeneral, L"parse json file '", bela::BaseName(file), L"' error: ",
                               bela::encode_into<char, wchar_t>(e.what()));
  }
  return std::nullopt;
}

class PathSearcher {
public:
  PathSearcher(const PathSearcher &) = delete;
  PathSearcher &operator=(const PathSearcher &) = delete;
  static PathSearcher &Instance() {
    static PathSearcher inst;
    return inst;
  }
  std::wstring JoinEtc(std::wstring_view p) { return (etc / p).wstring(); }
  std::wstring JoinPath(std::wstring_view p) { return (basePath / p).wstring(); }

private:
  std::filesystem::path etc;
  std::filesystem::path basePath;
  PathSearcher() {
    bela::error_code ec;
    if (!vfsInitialize(ec)) {
      etc = std::filesystem::path{L"."};
      basePath = std::filesystem::path{L"."};
    }
  }

  bool vfsInitialize(bela::error_code &ec) {
    auto binPath_ = bela::ExecutableParent(ec);
    if (!binPath_) {
      return false;
    }
    std::filesystem::path binPath{*binPath_};
    constexpr std::wstring_view baulkEnv{L"baulk.env"};
    basePath = binPath.parent_path();
    std::error_code e;
    for (int i = 0; i < 8; i++) {
      auto baulkEnvPath = basePath / baulkEnv;
      if (std::filesystem::exists(baulkEnvPath, e)) {
        return vfsInitializeFromEnv(baulkEnvPath.wstring(), ec);
      }
      basePath = basePath.parent_path();
    }
    return InitializeFromLegacy(binPath, ec);
  }
  bool vfsInitializeFromEnv(const std::wstring &file, bela::error_code &ec) {
    auto j = parse_file(file, ec);
    if (!j) {
      return false;
    }
    try {
      if (auto it = j->find("mode"); it != j->end() && it->is_string()) {
        auto mode = it->get<std::string>();
        if (!bela::EqualsIgnoreCase(mode, "Legacy")) {
          etc = basePath / L"etc";
          return true;
        }
      }
    } catch (const std::exception &) {
      return false;
    }
    etc = basePath / L"bin\\etc";
    return true;
  }

  bool InitializeFromLegacy(const std::filesystem::path &binPath, bela::error_code &ec) {
    constexpr std::wstring_view baulkExe{L"bin\\baulk.exe"};
    basePath = binPath.parent_path();
    std::error_code e;
    for (int i = 0; i < 8; i++) {
      auto baulkExePath = basePath / baulkExe;
      if (std::filesystem::exists(baulkExePath, e)) {
        etc = basePath / L"bin\\etc";
        return true;
      }
      basePath = basePath.parent_path();
    }
    return false;
  }
};

} // namespace example
#endif