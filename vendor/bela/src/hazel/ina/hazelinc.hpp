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
typedef enum hazel_status_e : int {
  None = 0,
  Found, ///
  Break
} status_t;
status_t LookupExecutableFile(bela::MemView mv, hazel::FileAttributeTable &fat);
status_t LookupArchives(bela::MemView mv, hazel::FileAttributeTable &fat);
status_t LookupDocs(bela::MemView mv, FileAttributeTable &fat);
status_t LookupFonts(bela::MemView mv, FileAttributeTable &fat);
status_t LookupShellLink(bela::MemView mv, FileAttributeTable &fat);
status_t LookupMedia(bela::MemView mv, FileAttributeTable &fat);
status_t LookupImages(bela::MemView mv, FileAttributeTable &fat);
status_t LookupText(bela::MemView mv, FileAttributeTable &fat);
void LookupShebang(const std::wstring_view line, FileAttributeTable &fat);
} // namespace hazel::internal

#endif