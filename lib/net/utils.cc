//
#include <baulk/net/types.hpp>

namespace baulk::net {

// decode url
std::string url_decode(std::wstring_view url) {
  auto u8url = bela::encode_into<wchar_t, char>(url);
  std::string buf;
  buf.reserve(u8url.size());
  auto p = u8url.data();
  auto len = u8url.size();
  for (size_t i = 0; i < len; i++) {
    if (auto ch = p[i]; ch != '%') {
      buf += static_cast<char>(ch);
      continue;
    }
    if (i + 3 > len) {
      return buf;
    }
    if (auto n = net_internal::decode_byte_couple(static_cast<uint8_t>(p[i + 1]), static_cast<uint8_t>(p[i + 2]));
        n > 0) {
      buf += static_cast<char>(n);
    }
    i += 2;
  }
  return buf;
}

} // namespace baulk::net