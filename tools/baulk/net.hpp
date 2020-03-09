//
#ifndef BAULK_NET_HPP
#define BAULK_NET_HPP
#include <bela/base.hpp>

namespace baulk::net {
std::optional<std::wstring> WinGet(std::wstring_view url,
                                   std::wstring_view workdir,
                                   bool forceoverwrite, bela::error_code ec);
} // namespace baulk::net

#endif