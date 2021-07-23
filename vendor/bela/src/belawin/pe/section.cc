//
#include "internal.hpp"
#include <charconv>

namespace bela::pe {

std::string File::sectionFullName(SectionHeader32 &sh) const {
  if (sh.Name[0] != '/') {
    return std::string(bela::cstring_view(sh.Name));
  }
  auto slen = bela::cstring_view({sh.Name + 1, sizeof(sh.Name) - 1});
  uint32_t offset = 0;
  if (auto result = std::from_chars(slen.data(), slen.data() + slen.size(), offset); result.ec != std::errc{}) {
    return "";
  }
  bela::error_code ec;
  return std::string(stringTable.make_cstring_view(bela::fromle(offset), ec));
}

bool File::readRelocs(Section &sec) const {
  if (sec.NumberOfRelocations == 0) {
    return true;
  }
  bela::error_code ec;
  sec.Relocs.resize(sec.NumberOfRelocations);
  if (!fd.ReadAt(sec.Relocs, sec.PointerToRelocations, ec)) {
    return false;
  }
  if constexpr (bela::IsBigEndian()) {
    for (auto &reloc : sec.Relocs) {
      reloc.VirtualAddress = bela::fromle(reloc.VirtualAddress);
      reloc.SymbolTableIndex = bela::fromle(reloc.SymbolTableIndex);
      reloc.Type = bela::fromle(reloc.Type);
    }
  }
  return true;
}
std::optional<Buffer> File::readSectionData(const Section &sec, bela::error_code &ec) const {
  if (sec.Size == 0) {
    return std::make_optional<Buffer>();
  }
  Buffer buffer(sec.Size);
  if (!fd.ReadAt(buffer, sec.Size, sec.Offset, ec)) {
    ec = bela::make_error_code(ec.code, L"unable read section data: ", ec.message);
    return std::nullopt;
  }
  return std::make_optional(std::move(buffer));
}
} // namespace bela::pe