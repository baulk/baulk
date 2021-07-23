//
#include "internal.hpp"

namespace bela::pe {

std::string symbolFullName(const COFFSymbol &sm, const StringTable &st) {
  if (sm.Name[0] == 0 && sm.Name[1] == 0 && sm.Name[2] == 0 && sm.Name[3] == 0) {
    auto offset = bela::cast_fromle<uint32_t>(sm.Name + 4);
    bela::error_code ec;
    return std::string(st.make_cstring_view(offset, ec));
  }
  return std::string(bela::cstring_view(sm.Name));
}

// Auxiliary Symbol Records
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
    sm.Name = symbolFullName(cm, st);
    sm.Value = cm.Value;
    sm.SectionNumber = cm.SectionNumber;
    sm.StorageClass = cm.StorageClass;
    sm.Type = cm.Type;
    syms.emplace_back(std::move(sm));
  }
  return true;
}

bool File::readCOFFSymbols(std::vector<COFFSymbol> &symbols, bela::error_code &ec) const {
  if (fh.PointerToSymbolTable == 0 || fh.NumberOfSymbols <= 0) {
    return true;
  }
  symbols.resize(fh.NumberOfSymbols);
  if (!fd.ReadAt(symbols, fh.PointerToSymbolTable, ec)) {
    return false;
  }
  if constexpr (bela::IsBigEndian()) {
    for (auto &s : symbols) {
      s.SectionNumber = bela::fromle(s.SectionNumber);
      s.Type = bela::fromle(s.Type);
      s.Value = bela::fromle(s.Value);
    }
  }
  return true;
}

// https://docs.microsoft.com/en-us/windows/win32/debug/pe-format#auxiliary-symbol-records
bool File::LookupSymbols(std::vector<Symbol> &syms, bela::error_code &ec) const {
  std::vector<COFFSymbol> csyms;
  if (!readCOFFSymbols(csyms, ec)) {
    return false;
  }
  return removeAuxSymbols(csyms, stringTable, syms, ec);
}

} // namespace bela::pe