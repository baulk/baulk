//
#include <version.hpp>
#include <bela/process.hpp>
#include <bela/str_split_narrow.hpp>
#include <bela/ascii.hpp>
#include <baulk/vfs.hpp>
#include <baulk/net.hpp>
#include <baulk/fs.hpp>
#include "executor.hpp"

namespace baulk {
constexpr bela::version zero_version;

constexpr std::wstring_view file_suffix_with_arch() {
#if defined(_M_X64)
  return L"win-x64.zip";
#elif defined(_M_ARM64)
  return L"win-arm64.zip";
#else
  return L"";
#endif
}

bool resolve_baulk_version(bela::version &version) {
  bela::process::Process ps;
  auto baulkexe = vfs::AppLocationPath(L"baulk-exec.exe");
  if (ps.Capture(baulkexe, L"--version") != 0) {
    if (ps.ErrorCode()) {
      bela::FPrintF(stderr, L"run %s error %s\n", baulkexe, ps.ErrorCode());
    }
    version = bela::version(BAULK_VERSION);
    return false;
  }
  // eg:
  // baulk-exec 4.0.0
  // Version:    4.0.0.1
  // Branch:     refs/heads/master
  // Commit:     7c0d17c334331d66ec3da1101fc76d3c6167e634
  // BuildTime:  none
  constexpr std::string_view releasePrefix = "Version:";
  std::vector<std::string_view> lines =
      bela::narrow::StrSplit(ps.Out(), bela::narrow::ByChar('\n'), bela::narrow::SkipEmpty());
  for (auto line : lines) {
    if (line.starts_with(releasePrefix)) {
      auto versionstr = bela::StripAsciiWhitespace(line.substr(releasePrefix.size()));
      if (version = bela::version(versionstr); version != zero_version) {
        return true;
      }
    }
  }
  return false;
}

bool Executor::latest_download_url() {
  bela::version version;
  if (!resolve_baulk_version(version)) {
    bela::FPrintF(stderr, L"baulk-update: \x1b[31munable detect baulk version\x1b[0m\n");
    return false;
  }
  if (IsDebugMode) {
    bela::FPrintF(stderr, L"\x1b[33m* current baulk version: %v\x1b[0m\n", version.make_string_version());
  }
  return latest_download_url(version);
}

bool Executor::latest_download_url(const bela::version &ov) {
  bela::error_code ec;
  auto resp = baulk::net::RestGet(L"https://api.github.com/repos/baulk/baulk/releases/latest", ec);
  if (!resp) {
    bela::FPrintF(stderr, L"baulk-update check latest version: \x1b[31m%s\x1b[0m\n", ec);
    return false;
  }
  try {
    auto obj = nlohmann::json::parse(resp->Content(), nullptr, true, true);
    auto tagname = bela::encode_into<char, wchar_t>(obj["tag_name"].get<std::string_view>());
    if (latest_version = bela::version(tagname); latest_version <= ov && !forceMode) {
      bela::FPrintF(stderr, L"\x1b[33mbaulk/%s is up to date\x1b[0m", tagname);
      return false;
    }
    DbgPrint(L"detect baulk latest version: %v", tagname);
    auto it = obj.find("assets");
    if (it == obj.end()) {
      bela::FPrintF(stderr, L"\x1b[33mbaulk/%s build is not yet complete\x1b[0m", tagname);
      return false;
    }
    for (const auto &p : it.value()) {
      auto name = bela::encode_into<char, wchar_t>(p["name"].get<std::string_view>());
      if (!bela::EndsWithIgnoreCase(name, file_suffix_with_arch())) {
        continue;
      }
      download_url = bela::encode_into<char, wchar_t>(p["browser_download_url"].get<std::string_view>());
      return true;
    }
  } catch (const std::exception &) {
    return latest_download_url_fallback(ov);
  }
  bela::FPrintF(stderr, L"baulk-update: \x1b[31munable check baulk latest version\x1b[0m\n");
  return false;
}

bool Executor::latest_download_url_fallback(const bela::version &ov) {
  constexpr std::wstring_view latest = L"https://github.com/baulk/baulk/releases/latest";
  constexpr std::string_view urlPrefix = "/baulk/baulk/releases/download/";
  // <a href="/baulk/baulk/releases/download/3.0.0/Baulk-3.0.0-win-arm64.zip" rel="nofollow" data-skip-pjax="">
  //             <span class="px-1 text-bold">Baulk-3.0.0-win-arm64.zip</span>
  //           </a>
  // constexpr std::string_view matchStringPrefix = R"(href="/baulk/baulk/releases/download/)";
  bela::error_code ec;
  auto resp = baulk::net::RestGet(latest, ec);
  if (!resp) {
    bela::FPrintF(stderr, L"baulk upgrade get latest: \x1b[31m%s\x1b[0m\n", ec);
    return false;
  }
  size_t index = 0;
  auto content = resp->Content();
  for (;;) {
    auto pos = content.find(urlPrefix, index);
    if (pos == std::wstring::npos) {
      bela::FPrintF(stderr, L"\x1b[33mbaulk/%s is up to date\x1b[0m\n", ov.make_string_version());
      return false;
    }
    auto pos2 = content.find('"', pos);
    if (pos2 == std::wstring::npos) {
      bela::FPrintF(stderr, L"baulk upgrade get %s: \x1b[31minvalid html url\x1b[0m\n", ov.make_string_version());
      return false;
    }
    auto filename = bela::encode_into<char, wchar_t>({content.data() + pos, pos2 - pos});

    index = pos2 + 1;
    std::vector<std::wstring_view> svv = bela::StrSplit(filename, bela::ByChar('/'), bela::SkipEmpty());
    if (svv.size() <= 2) {
      continue;
    }
    auto newrev = svv[svv.size() - 2];
    if (latest_version = bela::version(newrev); latest_version <= ov && !forceMode) {
      bela::FPrintF(stderr, L"\x1b[33mbaulk/%s is up to date\x1b[0m\n", ov.make_string_version());
      return false;
    }
    if (svv.back().find(file_suffix_with_arch()) != std::wstring_view::npos) {
      download_url = bela::StringCat(L"https://github.com", filename);
      DbgPrint(L"Found new release %s", filename);
      return true;
    }
  }
  bela::FPrintF(stderr, L"\x1b[33mbaulk/%s is up to date\x1b[0m\n", ov.make_string_version());
  return false;
}

std::optional<std::wstring> Executor::download_file() const {
  bela::error_code ec;
  if (!baulk::fs::MakeDir(vfs::AppTemp(), ec)) {
    bela::FPrintF(stderr, L"baulk unable mkdirall %s error: %s\n", vfs::AppTemp(), ec);
    return std::nullopt;
  }
  if (auto arfile = baulk::net::WinGet(download_url, vfs::AppTemp(), L"", true, ec); arfile) {
    return arfile;
  }
  bela::FPrintF(stderr, L"baulk download %s error: \x1b[31m%s\x1b[0m\n", download_url, ec);
  return std::nullopt;
}

} // namespace baulk