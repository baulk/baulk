///
#include "internal.hpp"

namespace bela::pe {
struct image_resource_directory {
  uint32_t characteristics;
  uint32_t time_date_stamp;
  uint16_t major_version;
  uint16_t minor_version;
  uint16_t number_of_named_entries;
  uint16_t number_of_id_entries;
  //  image_resource_directory_entry directory_entries[];
};

struct image_resource_directory_entry {
  union {
    struct {
      uint32_t name_offset : 31;
      uint32_t name_is_string : 1;
    };
    uint32_t name;
    uint16_t id;
  };
  union {
    uint32_t offset_to_data;
    struct {
      uint32_t offset_to_directory : 31;
      uint32_t data_is_directory : 1;
    };
  };
};

struct image_resource_directory_string {
  uint16_t length;
  uint8_t name_string[1];
};

struct image_resource_dir_string_u {
  uint16_t length;
  uint16_t name_string[1];
};

struct image_resource_data_entry {
  uint32_t offset_to_data;
  uint32_t size;
  uint32_t code_page;
  uint32_t reserved;
};

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