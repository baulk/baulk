// imports
#include "internal.hpp"
#include <algorithm>

namespace bela::pe {
bool File::LookupImports(FunctionTable::symbols_map_t &sm, bela::error_code &ec) const {
  auto idd = getDataDirectory(IMAGE_DIRECTORY_ENTRY_IMPORT);
  if (idd == nullptr) {
    return true;
  }
  auto ds = getSection(idd);
  if (ds == nullptr) {
    return true;
  }
  auto sdata = readSectionData(*ds, ec);
  if (!sdata) {
    return false;
  }
  auto bv = sdata->as_bytes_view();
  std::vector<image_import_descriptor> ida;
  for (size_t offset = idd->VirtualAddress - ds->VirtualAddress; offset < static_cast<size_t>(ds->Size);
       offset += sizeof(IMAGE_IMPORT_DESCRIPTOR)) {
    auto dt = bv.checked_cast<IMAGE_IMPORT_DESCRIPTOR>(offset);
    if (dt == nullptr) {
      break;
    }
    auto originalFirstThunk = bela::fromle(dt->OriginalFirstThunk);
    if (originalFirstThunk == 0) {
      break;
    }
    ida.emplace_back(image_import_descriptor{
        .OriginalFirstThunk = originalFirstThunk,
        .TimeDateStamp = bela::fromle(dt->TimeDateStamp),
        .ForwarderChain = bela::fromle(dt->ForwarderChain),
        .Name = bela::fromle(dt->Name),
        .FirstThunk = bela::fromle(dt->FirstThunk),
    });
  }
  for (auto &dt : ida) {
    auto dllName = bv.make_cstring_view(dt.Name - ds->VirtualAddress);
    std::vector<Function> functions;
    if (oh.Is64Bit) {
      for (size_t offset = dt.OriginalFirstThunk - ds->VirtualAddress; offset < static_cast<size_t>(ds->Size);
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
            bv.make_cstring_view(static_cast<size_t>(static_cast<uint64_t>(va) - ds->VirtualAddress + 2)),
            bv.cast_fromle<uint16_t>(static_cast<size_t>(static_cast<uint64_t>(va) - ds->VirtualAddress)));
      }
    } else {
      for (size_t offset = dt.OriginalFirstThunk - ds->VirtualAddress; offset < static_cast<size_t>(ds->Size);
           offset += sizeof(uint32_t)) {
        auto va = bv.cast_fromle<uint32_t>(offset);
        if (va == 0) {
          break;
        }
        if ((va & IMAGE_ORDINAL_FLAG32) != 0) {
          functions.emplace_back("", 0, static_cast<int>(IMAGE_ORDINAL32(va)));
          continue;
        }
        functions.emplace_back(bv.make_cstring_view(static_cast<size_t>(va - ds->VirtualAddress + 2)),
                               bv.cast_fromle<uint16_t>(static_cast<size_t>(va - ds->VirtualAddress)));
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