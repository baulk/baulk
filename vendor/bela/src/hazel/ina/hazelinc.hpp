//
#ifndef HAZEL_INTERNAL_INC_HPP
#define HAZEL_INTERNAL_INC_HPP
#include <hazel/hazel.hpp>
#include <bela/algorithm.hpp>
#include <bela/mapview.hpp>
#include <bela/endian.hpp>

namespace hazel::internal {
using bela::ArrayEnd;
using bela::ArrayEqual;
using bela::ArrayLength;
bool buffer_is_binary(bela::MemView mv);
typedef enum hazel_status_e : int {
  None = 0,
  Found, ///
  Break
} status_t;
status_t LookupExecutableFile(bela::MemView mv, hazel::hazel_result &hr);
status_t LookupArchives(bela::MemView mv, hazel::hazel_result &hr);
status_t LookupDocs(bela::MemView mv, hazel_result &hr);
status_t LookupFonts(bela::MemView mv, hazel_result &hr);
status_t LookupShellLink(bela::MemView mv, hazel_result &hr);
status_t LookupMedia(bela::MemView mv, hazel_result &hr);
status_t LookupImages(bela::MemView mv, hazel_result &hr);
status_t LookupText(bela::MemView mv, hazel_result &hr);
bool LookupShebang(const std::wstring_view line, hazel_result &hr);
} // namespace hazel::internal

#endif