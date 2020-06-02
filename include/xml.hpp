#ifndef BAULK_XML_HPP
#define BAULK_XML_HPP
#include <optional>
#include <bela/base.hpp>
#define PUGIXML_HEADER_ONLY 1
#include "../vendor/pugixml/pugixml.hpp"

namespace baulk::xml {
using document = pugi::xml_document;
inline std::optional<document> parse_string(const std::string_view buffer, bela::error_code &ec) {
  baulk::xml::document doc;
  auto result = doc.load_buffer(buffer.data(), buffer.size());
  if (!result) {
    ec = bela::make_error_code(1, bela::ToWide(result.description()));
    return std::nullopt;
  }
  return std::make_optional(std::move(doc));
}
} // namespace baulk::xml

#endif