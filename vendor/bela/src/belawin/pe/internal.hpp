///
#ifndef BELA_PE_INTERNAL_HPP
#define BELA_PE_INTERNAL_HPP
#include <bela/pe.hpp>

namespace bela::pe {

inline std::string_view cstring_view(const char *data, size_t len) {
  std::string_view sv{data, len};
  if (auto p = sv.find('\0'); p != std::string_view::npos) {
    return sv.substr(0, p);
  }
  return sv;
}

inline std::string_view cstring_view(const uint8_t *data, size_t len) {
  return cstring_view(reinterpret_cast<const char *>(data), len);
}

struct ImportDirectory {
  uint32_t OriginalFirstThunk;
  uint32_t TimeDateStamp;
  uint32_t ForwarderChain;
  uint32_t Name;
  uint32_t FirstThunk;

  std::string DllName;
};

struct ImportDelayDirectory {
  uint32_t Attributes;
  uint32_t DllNameRVA;
  uint32_t ModuleHandleRVA;
  uint32_t ImportAddressTableRVA;
  uint32_t ImportNameTableRVA;
  uint32_t BoundImportAddressTableRVA;
  uint32_t UnloadInformationTableRVA;
  uint32_t TimeDateStamp;

  std::string DllName;
};

std::string sectionFullName(SectionHeader32 &sh, StringTable &st);
bool readRelocs(Section &sec, FILE *fd);
bool readSectionData(std::vector<char> &data, const Section &sec, FILE *fd);
bool readStringTable(FileHeader *fh, FILE *fd, StringTable &table, bela::error_code &ec);
} // namespace bela::pe

#endif