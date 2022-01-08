#include "internal.hpp"
#include <algorithm>

namespace bela::pe {
// Delay imports
// https://docs.microsoft.com/en-us/windows/win32/debug/pe-format#delay-load-import-tables-image-only
bool File::LookupDelayImports(FunctionTable::symbols_map_t &sm, bela::error_code &ec) const {
  auto delay = getDataDirectory(IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT);
  if (delay == nullptr) {
    return true;
  }
  auto sec = getSection(delay);
  if (sec == nullptr) {
    return true;
  }
  auto sdata = readSectionData(*sec, ec);
  if (!sdata) {
    return false;
  }
  auto bv = sdata->as_bytes_view();
  // seek to the virtual address specified in the delay import data directory
  std::vector<image_delayload_descriptor> ida;
  for (size_t offset = delay->VirtualAddress - sec->VirtualAddress; offset < sec->Size;
       offset += sizeof(IMAGE_DELAYLOAD_DESCRIPTOR)) {
    auto dt = bv.checked_cast<IMAGE_DELAYLOAD_DESCRIPTOR>(offset);
    if (dt == nullptr) {
      break;
    }
    auto moduleHandleRVA = bela::fromle(dt->ModuleHandleRVA);
    if (moduleHandleRVA == 0) {
      break;
    }
    ida.emplace_back(
        image_delayload_descriptor{//
                                   .Attributes = bela::fromle(dt->Attributes.AllAttributes),
                                   .DllNameRVA = bela::fromle(dt->DllNameRVA),
                                   .ModuleHandleRVA = moduleHandleRVA,
                                   .ImportAddressTableRVA = bela::fromle(dt->ImportAddressTableRVA),
                                   .ImportNameTableRVA = bela::fromle(dt->ImportNameTableRVA),
                                   .BoundImportAddressTableRVA = bela::fromle(dt->BoundImportAddressTableRVA),
                                   .UnloadInformationTableRVA = bela::fromle(dt->UnloadInformationTableRVA),
                                   .TimeDateStamp = bela::fromle(dt->TimeDateStamp)} // C++20
    );
  }
  for (auto &dt : ida) {
    auto dllName = bv.make_cstring_view(dt.DllNameRVA - sec->VirtualAddress);
    if (dt.ImportNameTableRVA < sec->VirtualAddress || dt.ImportNameTableRVA > sec->VirtualAddress + sec->VirtualSize) {
      break;
    }

    std::vector<Function> functions;
    if (oh.Is64Bit) {
      for (size_t offset = dt.ImportNameTableRVA - sec->VirtualAddress; offset < static_cast<size_t>(sec->Size);
           offset += sizeof(uint64_t)) {
        auto va = bv.cast_fromle<uint64_t>(offset);
        if (va == 0) {
          break;
        }
        if ((va & IMAGE_ORDINAL_FLAG64) != 0) {
          functions.emplace_back("", 0, static_cast<int>(IMAGE_ORDINAL64(va)));
          continue;
        }
        functions.emplace_back(
            bv.make_cstring_view(static_cast<size_t>(static_cast<uint64_t>(va) - sec->VirtualAddress + 2)),
            bv.cast_fromle<uint16_t>(static_cast<size_t>(static_cast<uint64_t>(va) - sec->VirtualAddress)));
      }
    } else {
      for (size_t offset = dt.ImportNameTableRVA - sec->VirtualAddress; offset < static_cast<size_t>(sec->Size);
           offset += sizeof(uint32_t)) {
        auto va = bv.cast_fromle<uint32_t>(offset);
        if (va == 0) {
          break;
        }
        if ((va & IMAGE_ORDINAL_FLAG32) != 0) {
          functions.emplace_back("", 0, static_cast<int>(IMAGE_ORDINAL32(va)));
          continue;
        }
        functions.emplace_back(bv.make_cstring_view(static_cast<size_t>(va - sec->VirtualAddress + 2)),
                               bv.cast_fromle<uint16_t>(static_cast<size_t>(va - sec->VirtualAddress)));
      }
    }
    std::sort(functions.begin(), functions.end(), [](const bela::pe::Function &a, const bela::pe::Function &b) -> bool {
      return a.GetIndex() < b.GetIndex();
    });
    sm.emplace(dllName, std::move(functions));
  }
  return true;
}

} // namespace bela::pe