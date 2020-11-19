//
#include "internal.hpp"
#include <charconv>

namespace bela::pe {

std::string sectionFullName(SectionHeader32 &sh, StringTable &st) {
  if (sh.Name[0] != '/') {
    return std::string(cstring_view(sh.Name, sizeof(sh.Name)));
  }
  auto slen = cstring_view(sh.Name + 1, sizeof(sh.Name) - 1);
  uint32_t offset = 0;
  if (auto result = std::from_chars(slen.data(), slen.data() + slen.size(), offset); result.ec != std::errc{}) {
    return "";
  }
  bela::error_code ec;
  return st.String(offset, ec);
}

bool readRelocs(Section &sec, FILE *fd) {
  if (sec.Header.NumberOfRelocations == 0) {
    return true;
  }
  if (_fseeki64(fd, static_cast<int64_t>(sec.Header.PointerToRelocations), SEEK_SET) != 0) {
    return false;
  }
  sec.Relocs.resize(sec.Header.NumberOfRelocations);
  if (fread(sec.Relocs.data(), 1, sizeof(Reloc) * sec.Header.NumberOfRelocations, fd) !=
      sizeof(Reloc) * sec.Header.NumberOfRelocations) {
    return false;
  }
  if constexpr (bela::IsBigEndian()) {
    for (auto &reloc : sec.Relocs) {
      reloc.VirtualAddress = bela::swaple(reloc.VirtualAddress);
      reloc.SymbolTableIndex = bela::swaple(reloc.SymbolTableIndex);
      reloc.Type = bela::swaple(reloc.Type);
    }
  }
  return true;
}

bool readSectionData(std::vector<char> &data, const Section &sec, FILE *fd) {
  if (_fseeki64(fd, int64_t(sec.Header.Offset), SEEK_SET) != 0) {
    return false;
  }
  data.resize(sec.Header.Size);
  return fread(data.data(), 1, sec.Header.Size, fd) == sec.Header.Size;
}

} // namespace bela::pe