//
#ifndef HAZEL_HAZEL_HPP
#define HAZEL_HAZEL_HPP
#include <bela/base.hpp>
#include <bela/phmap.hpp>
#include <bela/buffer.hpp>
#include "types.hpp"
#include "io.hpp"

namespace hazel {

// file attribute table
struct FileAttributeTable {
  bela::flat_hash_map<std::wstring, std::wstring> attributes;
  bela::flat_hash_map<std::wstring, std::vector<std::wstring>> multi_attributes;
  int64_t size{0};
  types::hazel_types_t type{types::none};
  FileAttributeTable &assign(std::wstring_view desc, types::hazel_types_t t = types::none) {
    attributes.emplace(L"Description", desc);
    type = t;
    return *this;
  }
  FileAttributeTable &append(const std::wstring_view &name, const std::wstring_view &value) {
    attributes.emplace(name, value);
    return *this;
  }
  FileAttributeTable &append(std::wstring &&name, std::wstring &&value) {
    attributes.emplace(std::move(name), std::move(value));
    return *this;
  }
  FileAttributeTable &append(std::wstring &&name, std::vector<std::wstring> &&value) {
    multi_attributes.emplace(std::move(name), std::move(value));
    return *this;
  }

  bool LooksLikeELF() const {
    return type == types::elf || type == types::elf_executable || type == types::elf_relocatable ||
           type == types::elf_shared_object;
  }
  bool LooksLikeMachO() const {
    return type == types::macho_bundle || type == types::macho_core || type == types::macho_dsym_companion ||
           type == types::macho_dynamic_linker || type == types::macho_dynamically_linked_shared_lib ||
           type == types::macho_dynamically_linked_shared_lib_stub || type == types::macho_executable ||
           type == types::macho_fixed_virtual_memory_shared_lib || type == types::macho_kext_bundle ||
           type == types::macho_object || type == types::macho_preload_executable ||
           type == types::macho_universal_binary;
  }
  bool LooksLikePE() const { return type == types::pecoff_executable; }
  bool LooksLikeZIP() const {
    return type == types::zip || type == types::docx || type == types::xlsx || type == types::pptx ||
           type == types::ofd;
  }
};

bool LookupFile(hazel::io::File &fd, FileAttributeTable &fat, bela::error_code &ec);
} // namespace hazel

#endif