//
#ifndef BAULK_BUCKET_HPP
#define BAULK_BUCKET_HPP
#include <string>
#include <optional>

namespace baulk::bucket {
std::optional<std::wstring> BuacketNewest(std::wstring_view rss);
}

#endif