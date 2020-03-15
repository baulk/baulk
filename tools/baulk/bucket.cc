//
#include "bucket.hpp"
#include "net.hpp"
#include <xml.hpp>
#include "jsonex.hpp"

namespace baulk::bucket {
//
std::optional<std::wstring> BuacketNewest(std::wstring_view rss,
                                          bela::error_code &ec) {
  auto resp = baulk::net::RestGet(rss, ec);
  if (!resp) {
    return std::nullopt;
  }
  auto doc = baulk::xml::parse_string(resp->body, ec);
  if (!doc) {
    return std::nullopt;
  }
  // first entry child
  auto entry = doc->child("feed").child("entry");
  std::string_view id{entry.child("id").text().as_string()};
  if (auto pos = id.find('/'); pos != std::string_view::npos) {
    return std::make_optional(bela::ToWide(id.substr(pos + 1)));
  }
  ec = bela::make_error_code(1, L"bucket invaild id: ", bela::ToWide(id));
  return std::nullopt;
}

} // namespace baulk::bucket