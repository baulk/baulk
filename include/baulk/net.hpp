//
#ifndef BAULK_NET_HPP
#define BAULK_NET_HPP
#include <baulk/net/types.hpp>
#include <baulk/net/client.hpp>
#include <baulk/net/tcp.hpp>

namespace baulk::net {
std::wstring HijackGithubUrl(std::wstring_view url);
std::wstring BestUrl(const std::vector<std::wstring> &urls, std::wstring_view locale);
} // namespace baulk::net

#endif
