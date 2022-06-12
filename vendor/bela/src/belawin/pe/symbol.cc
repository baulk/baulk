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
  for (const auto &s : csyms) {
    if (aux > 0) {
      aux--;
      continue;
    }
    aux = s.NumberOfAuxSymbols;
    syms.emplace_back(Symbol{.Name = symbolFullName(s, st),
                             .Value = bela::fromle(s.Value),
                             .SectionNumber = bela::fromle(s.SectionNumber),
                             .Type = bela::fromle(s.Type),
                             .StorageClass = s.StorageClass});
  }
  return true;
}

// readCOFFSymbols reads in the symbol table for a PE file, returning
// a slice of COFFSymbol objects. The PE format includes both primary
// symbols (whose fields are described by COFFSymbol above) and
// auxiliary symbols; all symbols are 18 bytes in size. The auxiliary
// symbols for a given primary symbol are placed following it in the
// array, e.g.
//
//   ...
//   k+0:  regular sym k
//   k+1:    1st aux symbol for k
//   k+2:    2nd aux symbol for k
//   k+3:  regular sym k+3
//   k+4:    1st aux symbol for k+3
//   k+5:  regular sym k+5
//   k+6:  regular sym k+6
//
// The PE format allows for several possible aux symbol formats. For
// more info see:
//
//     https://docs.microsoft.com/en-us/windows/win32/debug/pe-format#auxiliary-symbol-records
//
// At the moment this package only provides APIs for looking at
// aux symbols of format 5 (associated with section definition symbols).
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

// COFFSymbolReadSectionDefAux returns a blob of axiliary information
// (including COMDAT info) for a section definition symbol. Here 'idx'
// is the index of a section symbol in the main COFFSymbol array for
// the File. Return value is a pointer to the appropriate aux symbol
// struct. For more info, see:
//
// auxiliary symbols: https://docs.microsoft.com/en-us/windows/win32/debug/pe-format#auxiliary-symbol-records
// COMDAT sections: https://docs.microsoft.com/en-us/windows/win32/debug/pe-format#comdat-sections-object-only
// auxiliary info for section definitions:
// https://docs.microsoft.com/en-us/windows/win32/debug/pe-format#auxiliary-format-5-section-definitions
//

// https://docs.microsoft.com/en-us/windows/win32/debug/pe-format#auxiliary-symbol-records
bool File::LookupSymbols(std::vector<Symbol> &syms, bela::error_code &ec) const {
  std::vector<COFFSymbol> csyms;
  if (!readCOFFSymbols(csyms, ec)) {
    return false;
  }
  return removeAuxSymbols(csyms, stringTable, syms, ec);
}

} // namespace bela::pe