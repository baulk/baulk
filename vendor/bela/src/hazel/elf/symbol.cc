///
#include <hazel/elf.hpp>

namespace hazel::elf {

constexpr size_t Sym64Size = sizeof(Elf64_Sym);
constexpr size_t Sym32Size = sizeof(Elf32_Sym);

bool File::getSymbols32(uint32_t st, std::vector<Symbol> &syms, bela::Buffer &strdata, bela::error_code &ec) const {
  auto symSec = SectionByType(st);
  if (symSec == nullptr) {
    ec = bela::make_error_code(L"no symbol section");
    return false;
  }
  bela::Buffer buffer;
  if (!sectionData(*symSec, buffer, ec)) {
    return false;
  }
  if (buffer.size() % Sym32Size != 0) {
    ec = bela::make_error_code(L"length of symbol section is not a multiple of SymSize");
    return false;
  }
  if (!stringTable(symSec->Link, strdata, ec)) {
    return false;
  }
  auto bv = strdata.as_bytes_view();
  auto bsv = buffer.as_bytes_view();
  if (bsv.size() > Sym32Size) {
    bsv.remove_prefix(Sym32Size);
  }
  syms.resize(bsv.size() / Sym32Size);
  for (auto &symbol : syms) {
    auto sym = bsv.unchecked_cast<Elf32_Sym>();
    bsv.remove_prefix(Sym32Size);
    symbol.Name = bv.make_cstring_view(endian_cast(sym->st_name));
    symbol.Info = endian_cast(sym->st_info);
    symbol.Other = endian_cast(sym->st_other);
    symbol.Value = endian_cast(sym->st_shndx);
    symbol.Size = endian_cast(sym->st_size);
  }
  return true;
}

bool File::getSymbols64(uint32_t st, std::vector<Symbol> &syms, bela::Buffer &strdata, bela::error_code &ec) const {
  auto symSec = SectionByType(st);
  if (symSec == nullptr) {
    ec = bela::make_error_code(L"no symbol section");
    return false;
  }
  bela::Buffer buffer;
  if (!sectionData(*symSec, buffer, ec)) {
    return false;
  }
  if (buffer.size() % Sym64Size != 0) {
    ec = bela::make_error_code(L"length of symbol section is not a multiple of SymSize");
    return false;
  }
  if (!stringTable(symSec->Link, strdata, ec)) {
    return false;
  }
  auto bv = strdata.as_bytes_view();
  auto bsv = buffer.as_bytes_view();
  std::string_view symtab{reinterpret_cast<const char *>(buffer.data()), buffer.size()};
  if (symtab.size() > Sym64Size) {
    symtab.remove_prefix(Sym64Size);
  }
  syms.resize(symtab.size() / Sym64Size);
  for (auto &symbol : syms) {
    auto sym = bsv.unchecked_cast<Elf64_Sym>();
    bsv.remove_prefix(Sym64Size);
    symbol.Name = bv.make_cstring_view(endian_cast(sym->st_name));
    symbol.Info = endian_cast(sym->st_info);
    symbol.Other = endian_cast(sym->st_other);
    symbol.Value = endian_cast(sym->st_shndx);
    symbol.Size = endian_cast(sym->st_size);
  }
  return true;
}
} // namespace hazel::elf