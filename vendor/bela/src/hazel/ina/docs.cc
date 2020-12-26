//////////////
#include "hazelinc.hpp"
#include "docs.hpp"
#include <charconv>
#include <bela/ascii.hpp>

namespace hazel::internal {

// RTF format
// https://en.wikipedia.org/wiki/Rich_Text_Format
/*{\rtf1*/
status_t lookup_rtfinternal(bela::MemView mv, hazel_result &hr) {
  constexpr uint8_t rtfMagic[] = {0x7B, 0x5C, 0x72, 0x74, 0x66};
  if (!mv.StartsWith(rtfMagic) || mv.size() < 6) {
    return None;
  }
  // {\rtf1
  auto sv = mv.submv(5).sv();
  int version = 0;
  if (auto result = std::from_chars(sv.data(), sv.data() + sv.size(), version); result.ec != std::errc{}) {
    return None;
  }
  hr.assign(types::rtf, L"Rich Text Format");
  hr.append(L"Version", version);
  return Found;
}

// http://www.openoffice.org/sc/compdocfileformat.pdf
// https://interoperability.blob.core.windows.net/files/MS-PPT/[MS-PPT].pdf

status_t LookupDocs(bela::MemView mv, hazel_result &hr) {
  constexpr const uint8_t msofficeMagic[] = {0xD0, 0xCF, 0x11, 0xE0, 0xA1, 0xB1, 0x1A, 0xE1};
  constexpr const uint8_t pptMagic[] = {0xD0, 0xCF, 0x11, 0xE0, 0xA1, 0xB1, 0x1A, 0xE1, 0x00, 0x00, 0x00, 0x00};
  constexpr const uint8_t wordMagic[] = {0xD0, 0xCF, 0x11, 0xE0, 0xA1, 0xB1, 0x1A, 0xE1, 0x00};
  constexpr const uint8_t xlsMagic[] = {0xD0, 0xCF, 0x11, 0xE0, 0xA1, 0xB1, 0x1A, 0xE1, 0x00};
  if (lookup_rtfinternal(mv, hr) == Found) {
    return Found;
  }
  constexpr const auto olesize = sizeof(oleheader_t);
  if (mv.StartsWith(msofficeMagic) || mv.size() < 512) {
    return None;
  }
  oleheader_t hdr;
  auto oh = mv.bit_cast<oleheader_t>(&hdr);
  if (oh == nullptr) {
    return None;
  }
  // PowerPoint Document

  return None;
}
} // namespace hazel::internal
