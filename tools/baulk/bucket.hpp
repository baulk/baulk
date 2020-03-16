//
#ifndef BAULK_BUCKET_HPP
#define BAULK_BUCKET_HPP
#include <string>
#include <optional>
#include <bela/base.hpp>
#include "baulk.hpp"

namespace baulk::bucket {
std::optional<std::wstring> BuacketNewest(std::wstring_view rss,
                                          bela::error_code &ec);
}

#endif