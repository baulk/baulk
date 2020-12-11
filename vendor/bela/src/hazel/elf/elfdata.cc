//
#include <hazel/elf.hpp>

namespace bela::elf {

const char *ElfABIName(int abi) {
  static constexpr struct {
    int id;
    const char *name;
  } abinames[] = {
      {0, "ELFOSABI_NONE"},     {1, "ELFOSABI_HPUX"},      {2, "ELFOSABI_NETBSD"},  {3, "ELFOSABI_LINUX"},
      {4, "ELFOSABI_HURD"},     {5, "ELFOSABI_86OPEN"},    {6, "ELFOSABI_SOLARIS"}, {7, "ELFOSABI_AIX"},
      {8, "ELFOSABI_IRIX"},     {9, "ELFOSABI_FREEBSD"},   {10, "ELFOSABI_TRU64"},  {11, "ELFOSABI_MODESTO"},
      {12, "ELFOSABI_OPENBSD"}, {13, "ELFOSABI_OPENVMS"},  {14, "ELFOSABI_NSK"},    {15, "ELFOSABI_AROS"},
      {16, "ELFOSABI_FENIXOS"}, {17, "ELFOSABI_CLOUDABI"}, {97, "ELFOSABI_ARM"},    {255, "ELFOSABI_STANDALONE"},
  };
  for (const auto &e : abinames) {
    if (e.id == abi) {
      return e.name;
    }
  }
  return "ELFOSABI_ERROR";
}

const char *SectionName(uint32_t t) {
  static constexpr struct {
    uint32_t i;
    const char *name;
  } shtStrings[] = {
      {0, "SHT_NULL"},
      {1, "SHT_PROGBITS"},
      {2, "SHT_SYMTAB"},
      {3, "SHT_STRTAB"},
      {4, "SHT_RELA"},
      {5, "SHT_HASH"},
      {6, "SHT_DYNAMIC"},
      {7, "SHT_NOTE"},
      {8, "SHT_NOBITS"},
      {9, "SHT_REL"},
      {10, "SHT_SHLIB"},
      {11, "SHT_DYNSYM"},
      {14, "SHT_INIT_ARRAY"},
      {15, "SHT_FINI_ARRAY"},
      {16, "SHT_PREINIT_ARRAY"},
      {17, "SHT_GROUP"},
      {18, "SHT_SYMTAB_SHNDX"},
      {0x60000000, "SHT_LOOS"},
      {0x6ffffff5, "SHT_GNU_ATTRIBUTES"},
      {0x6ffffff6, "SHT_GNU_HASH"},
      {0x6ffffff7, "SHT_GNU_LIBLIST"},
      {0x6ffffffd, "SHT_GNU_VERDEF"},
      {0x6ffffffe, "SHT_GNU_VERNEED"},
      {0x6fffffff, "SHT_GNU_VERSYM"},
      {0x70000000, "SHT_LOPROC"},
      {0x7fffffff, "SHT_HIPROC"},
      {0x80000000, "SHT_LOUSER"},
      {0xffffffff, "SHT_HIUSER"},
  };
  for (const auto &s : shtStrings) {
    if (s.i == t) {
      return s.name;
    }
  }
  return "";
}

} // namespace bela::elf