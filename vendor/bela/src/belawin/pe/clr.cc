///
#include "internal.hpp"

namespace bela::pe {
typedef enum ReplacesGeneralNumericDefines {
// Directory entry macro for CLR data.
#ifndef IMAGE_DIRECTORY_ENTRY_COMHEADER
  IMAGE_DIRECTORY_ENTRY_COMHEADER = 14,
#endif // IMAGE_DIRECTORY_ENTRY_COMHEADER
} ReplacesGeneralNumericDefines;
#define STORAGE_MAGIC_SIG 0x424A5342 // BSJB

bool File::LookupClrVersion(std::string &ver, bela::error_code &ec) const {
  uint32_t ddlen = 0;
  const DataDirectory *clrd = nullptr;
  if (is64bit) {
    ddlen = oh.NumberOfRvaAndSizes;
    clrd = &(oh.DataDirectory[IMAGE_DIRECTORY_ENTRY_COMHEADER]);
  } else {
    auto oh3 = reinterpret_cast<const OptionalHeader32 *>(&oh);
    ddlen = oh3->NumberOfRvaAndSizes;
    clrd = &(oh3->DataDirectory[IMAGE_DIRECTORY_ENTRY_COMHEADER]);
  }
  if (ddlen < IMAGE_DIRECTORY_ENTRY_IMPORT + 1 || clrd->VirtualAddress == 0) {
    return true;
  }
  const Section *ds = nullptr;
  for (const auto &sec : sections) {
    if (sec.Header.VirtualAddress <= clrd->VirtualAddress &&
        clrd->VirtualAddress < sec.Header.VirtualAddress + sec.Header.VirtualSize) {
      ds = &sec;
    }
  }
  if (ds == nullptr) {
    return true;
  }

  std::vector<char> sdata;
  if (!readSectionData(*ds, sdata)) {
    ec = bela::make_error_code(L"unable read section data");
    return false;
  }
  auto N = clrd->VirtualAddress - ds->Header.VirtualAddress;
  std::string_view sv{sdata.data() + N, sdata.size() - N};
  if (sv.size() < sizeof(IMAGE_COR20_HEADER)) {
    return false;
  }
  auto cr = reinterpret_cast<const IMAGE_COR20_HEADER *>(sv.data());
  N = cr->MetaData.VirtualAddress - ds->Header.VirtualAddress;
  std::string_view sv2{sdata.data() + N, sdata.size() - N};
  if (sv.size() < sizeof(STORAGESIGNATURE)) {
    return false;
  }
  auto d = reinterpret_cast<const STORAGESIGNATURE *>(sv2.data());
  STORAGESIGNATURE ssi = {0};
  if (bela::swaple(d->lSignature) != STORAGE_MAGIC_SIG) {
    return false;
  }
  ver = getString(sdata, N + sizeof(STORAGESIGNATURE));
  return true;
}

} // namespace bela::pe