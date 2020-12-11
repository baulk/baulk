///
#include "internal.hpp"

namespace hazel::elf {

constexpr size_t Sym64Size = sizeof(Elf64_Sym);
constexpr size_t Sym32Size = sizeof(Elf32_Sym);

bool File::getSymbols32(uint32_t st, std::vector<Symbol> &syms, bela::Buffer &strdata, bela::error_code &ec) {
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
  std::string_view symtab{reinterpret_cast<const char *>(buffer.data()), buffer.size()};
  if (symtab.size() > Sym32Size) {
    symtab.remove_prefix(Sym32Size);
  }
  syms.resize(symtab.size() / Sym32Size);
  int i = 0;
  while (symtab.size() >= Sym32Size) {
    auto sym = reinterpret_cast<const Elf32_Sym *>(symtab.data());
    auto symbol = &syms[i];
    symbol->Name = getString(strdata.Span(), EndianCast(sym->st_name));
    symbol->Info = EndianCast(sym->st_info);
    symbol->Other = EndianCast(sym->st_other);
    symbol->Value = EndianCast(sym->st_shndx);
    symbol->Size = EndianCast(sym->st_size);
    symtab.remove_prefix(Sym32Size);
    i++;
    symtab.remove_prefix(Sym32Size);
  }
  return true;
}

bool File::getSymbols64(uint32_t st, std::vector<Symbol> &syms, bela::Buffer &strdata, bela::error_code &ec) {
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
  std::string_view symtab{reinterpret_cast<const char *>(buffer.data()), buffer.size()};
  if (symtab.size() > Sym64Size) {
    symtab.remove_prefix(Sym64Size);
  }
  syms.resize(symtab.size() / Sym64Size);
  int i = 0;
  while (symtab.size() >= Sym64Size) {
    auto sym = reinterpret_cast<const Elf64_Sym *>(symtab.data());
    auto symbol = &syms[i];
    symbol->Name = getString(strdata.Span(), EndianCast(sym->st_name));
    symbol->Info = EndianCast(sym->st_info);
    symbol->Other = EndianCast(sym->st_other);
    symbol->Value = EndianCast(sym->st_shndx);
    symbol->Size = EndianCast(sym->st_size);
    symtab.remove_prefix(Sym32Size);
    i++;
    symtab.remove_prefix(Sym32Size);
  }
  return true;
}

} // namespace hazel::elf