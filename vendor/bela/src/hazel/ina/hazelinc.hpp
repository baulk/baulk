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
status_t LookupExecutableFile(bela::bytes_view bv, hazel::hazel_result &hr);
status_t LookupArchives(bela::bytes_view bv, hazel::hazel_result &hr);
status_t LookupDocs(bela::bytes_view bv, hazel_result &hr);
status_t LookupFonts(bela::bytes_view bv, hazel_result &hr);
status_t LookupShellLink(bela::bytes_view bv, hazel_result &hr);
status_t LookupMedia(bela::bytes_view bv, hazel_result &hr);
status_t LookupImages(bela::bytes_view bv, hazel_result &hr);
status_t LookupText(bela::bytes_view bv, hazel_result &hr);
bool LookupShebang(const std::wstring_view line, hazel_result &hr);
} // namespace hazel::internal

#endif