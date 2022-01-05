//
#include <baulk/net/types.hpp>

namespace baulk::net {

// decode url
std::wstring url_decode(std::wstring_view str) {
  std::wstring buf;
  buf.reserve(str.size());
  auto p = str.data();
  auto len = str.size();
  for (size_t i = 0; i < len; i++) {
    if (auto ch = p[i]; ch != '%') {
      buf += static_cast<wchar_t>(ch);
      continue;
    }
    if (i + 3 > len) {
      return buf;
    }
    if (auto n = net_internal::decode_byte_couple(static_cast<uint8_t>(p[i + 1]), static_cast<uint8_t>(p[i + 2]));
        n > 0) {
      buf += static_cast<wchar_t>(n);
    }
    i += 2;
  }
  return buf;
}

} // namespace baulk::net