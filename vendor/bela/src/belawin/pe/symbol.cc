//
#include "internal.hpp"

namespace bela::pe {

std::string symbolFullName(const COFFSymbol &sm, const StringTable &st) {
  if (sm.Name[0] == 0 && sm.Name[1] == 0 && sm.Name[2] == 0 && sm.Name[3] == 0) {
    auto offset = bela::readle<uint32_t>(sm.Name + 4);
    bela::error_code ec;
    return st.String(offset, ec);
  }
  return std::string(cstring_view(sm.Name, sizeof(sm.Name)));
}

bool readCOFFSymbols(const FileHeader &fh, FILE *fd, std::vector<COFFSymbol> &symbols, bela::error_code &ec) {
  if (fh.PointerToSymbolTable == 0 || fh.NumberOfSymbols <= 0) {
    return true;
  }
  if (auto eno = _fseeki64(fd, int64_t(fh.PointerToSymbolTable), SEEK_SET); eno != 0) {
    ec = bela::make_stdc_error_code(eno, L"fail to seek to symbol table: ");
    return false;
  }
  symbols.resize(fh.NumberOfSymbols);
  if (fread(symbols.data(), 1, sizeof(COFFSymbol) * fh.NumberOfSymbols, fd) !=
      sizeof(COFFSymbol) * fh.NumberOfSymbols) {
    ec = bela::make_stdc_error_code(ferror(fd), L"fail to read symbol table: ");
    return false;
  }
  if constexpr (bela::IsBigEndian()) {
    for (auto &s : symbols) {
      s.SectionNumber = bela::swaple(s.SectionNumber);
      s.Type = bela::swaple(s.Type);
      s.Value = bela::swaple(s.Value);
    }
  }
  return true;
}

bool removeAuxSymbols(const std::vector<COFFSymbol> &csyms, const StringTable &st, std::vector<Symbol> &syms,
                      bela::error_code &ec) {
  if (csyms.empty()) {
    return true;
  }
  uint8_t aux = 0;
  for (const auto &cm : csyms) {
    if (aux > 0) {
      aux--;
      continue;
    }
    aux = cm.NumberOfAuxSymbols;
    Symbol sm;
    sm.Name = std::move(symbolFullName(cm, st));
    sm.Value = cm.Value;
    sm.SectionNumber = cm.SectionNumber;
    sm.StorageClass = cm.StorageClass;
    sm.Type = cm.Type;
    syms.emplace_back(std::move(sm));
  }
  return true;
}

bool File::LookupSymbols(std::vector<Symbol> &syms, bela::error_code &ec) const {
  std::vector<COFFSymbol> csyms;
  if (!readCOFFSymbols(fh, fd, csyms, ec)) {
    return false;
  }
  return removeAuxSymbols(csyms, stringTable, syms, ec);
}

} // namespace bela::pe