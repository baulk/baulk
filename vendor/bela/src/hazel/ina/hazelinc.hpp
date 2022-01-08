//
#ifndef HAZEL_INTERNAL_INC_HPP
#define HAZEL_INTERNAL_INC_HPP
#include <hazel/hazel.hpp>
#include <bela/endian.hpp>

namespace hazel::internal {
typedef enum hazel_status_e : int {
  None = 0,
  Found, ///
  Break
} status_t;
status_t LookupExecutableFile(const bela::bytes_view &bv, hazel::hazel_result &hr);
status_t LookupArchives(const bela::bytes_view &bv, hazel::hazel_result &hr);
status_t LookupDocs(const bela::bytes_view &bv, hazel_result &hr);
status_t LookupFonts(const bela::bytes_view &bv, hazel_result &hr);
status_t LookupShellLink(const bela::bytes_view &bv, hazel_result &hr);
status_t LookupMedia(const bela::bytes_view &bv, hazel_result &hr);
status_t LookupImages(const bela::bytes_view &bv, hazel_result &hr);
status_t LookupText(const bela::bytes_view &bv, hazel_result &hr);
bool LookupShebang(const std::wstring_view line, hazel_result &hr);
} // namespace hazel::internal

#endif