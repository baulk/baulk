//
#include <baulk/net.hpp>
#include <baulk/net/tcp.hpp>
#include "native.hpp"

namespace baulk::net {
constexpr auto MaximumTime = (std::numeric_limits<std::uint64_t>::max)();

std::uint64_t UrlResponseTime(std::wstring_view url) {
  bela::error_code ec;
  auto u = native::crack_url(url, ec);
  if (!u) {
    return MaximumTime;
  }
  auto begin = std::chrono::steady_clock::now();
  if (auto conn = baulk::net::DialTimeout(u->host, u->nPort, 10000, ec); !conn) {
    return MaximumTime;
  }
  auto cur = std::chrono::steady_clock::now();
  return std::chrono::duration_cast<std::chrono::nanoseconds>(cur - begin).count();
}

std::wstring HijackGithubUrl(std::wstring_view url) {
  if (auto ghProxy = bela::GetEnv(L"GITHUB_PROXY"); !ghProxy.empty()) {
    bela::error_code ec;
    if (auto u = native::crack_url(url, ec); u && bela::EqualsIgnoreCase(u->host, L"github.com")) {
      return bela::StringCat(ghProxy, url);
    }
  }
  return std::wstring(url);
}

std::wstring_view BestUrlInternal(const std::vector<std::wstring> &urls, std::wstring_view locale) {
  if (urls.empty()) {
    return L"";
  }
  if (urls.size() == 1) {
    return urls[0];
  }
  auto elapsed = MaximumTime;
  size_t pos = 0;
  auto suffix = bela::StringCat(L"#", locale);
  // The first round to determine whether there is a mirror image of the area
  for (const auto &u : urls) {
    std::wstring_view url(u);
    if (bela::EndsWithIgnoreCase(url, suffix)) {
      // eg: https://npm.taobao.org/.../MinGit-2.26.0-busybox-64-bit.zip#zh-CN
      url.remove_suffix(suffix.size());
      return url;
    }
  }
  // Second round of analysis of network connection establishment time
  for (size_t i = 0; i < urls.size(); i++) {
    // connect url to get elapsed timeout
    auto resptime = UrlResponseTime(urls[i]);
    if (resptime < elapsed) {
      pos = i;
    }
  }
  return urls[pos];
}

std::wstring BestUrl(const std::vector<std::wstring> &urls, std::wstring_view locale) {
  auto url = BestUrlInternal(urls, locale);
  if (auto pos = url.find('#'); pos != std::wstring_view::npos) {
    return HijackGithubUrl(url.substr(0, pos));
  }
  return HijackGithubUrl(url);
}
} // namespace baulk::net