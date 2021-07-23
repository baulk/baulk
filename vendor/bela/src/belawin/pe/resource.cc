///
#include "internal.hpp"

namespace bela::pe {
// manifest
constexpr std::string_view rsrcName = ".rsrc";

std::optional<Version> File::LookupVersion(bela::error_code &ec) const {
  auto dd = getDataDirectory(IMAGE_DIRECTORY_ENTRY_RESOURCE);
  if (dd == nullptr) {
    return std::nullopt;
  }
  if (dd->VirtualAddress == 0) {
    return std::nullopt;
  }
  auto sec = getSection(rsrcName);
  if (sec == nullptr) {
    return std::nullopt;
  }
  auto offset = static_cast<int64_t>(sec->Offset);
  auto N = dd->VirtualAddress - sec->VirtualAddress;
  auto offsetSize = sec->Size - N;
  IMAGE_RESOURCE_DIRECTORY ird;
  if (!fd.ReadAt(ird, offset, ec)) {
    return std::nullopt;
  }
  auto totalEntries = static_cast<int>(ird.NumberOfNamedEntries) + static_cast<int>(ird.NumberOfIdEntries);
  if (totalEntries == 0) {
    return std::nullopt;
  }
  IMAGE_RESOURCE_DIRECTORY_ENTRY entry;
  for (auto i = 0; i < totalEntries; i++) {
    if (!fd.ReadFull(entry, ec)) {
      return std::nullopt;
    }
    // if (entry.NameIsString != 1) {
    //   continue;
    // }
    // if (entry.Name != 0x00000010) {
    // }
  }
  return std::nullopt;
}

} // namespace bela::pe