//
#include "internal.hpp"
#include <charconv>

namespace bela::pe {

std::string File::sectionFullName(SectionHeader32 &sh) const {
  if (sh.Name[0] != '/') {
    return std::string(cstring_view(sh.Name, sizeof(sh.Name)));
  }
  auto slen = cstring_view(sh.Name + 1, sizeof(sh.Name) - 1);
  uint32_t offset = 0;
  if (auto result = std::from_chars(slen.data(), slen.data() + slen.size(), offset); result.ec != std::errc{}) {
    return "";
  }
  bela::error_code ec;
  return stringTable.String(offset, ec);
}

bool File::readRelocs(Section &sec) const {
  if (sec.Header.NumberOfRelocations == 0) {
    return true;
  }
  bela::error_code ec;
  sec.Relocs.resize(sec.Header.NumberOfRelocations);
  if (!ReadAt(sec.Relocs.data(), sizeof(Reloc) * sec.Header.NumberOfRelocations, sec.Header.PointerToRelocations, ec)) {
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

bool File::readSectionData(const Section &sec, std::vector<char> &data) const {
  bela::error_code ec;
  data.resize(sec.Header.Size);
  return ReadAt(data.data(), sec.Header.Size, sec.Header.Offset, ec);
}

} // namespace bela::pe