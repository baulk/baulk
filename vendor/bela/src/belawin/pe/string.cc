///
#include "internal.hpp"

namespace bela::pe {

void StringTable::MoveFrom(StringTable &&other) {
  if (data != nullptr) {
    HeapFree(GetProcessHeap(), 0, data);
  }
  data = other.data;
  length = other.length;
  other.data = nullptr;
  other.length = 0;
}

StringTable::~StringTable() { HeapFree(GetProcessHeap(), 0, data); }

std::string StringTable::String(uint32_t start, bela::error_code &ec) const {
  if (start < 4) {
    ec = bela::make_error_code(1, L"offset ", start, L" is before the start of string table");
    return "";
  }
  start -= 4;
  if (static_cast<size_t>(start) > length) {
    ec = bela::make_error_code(1, L"offset ", start, L" is beyond the end of string table");
    return "";
  }
  return std::string(cstring_view(data + start, length - start));
}

bool File::readStringTable(bela::error_code &ec) {
  if (stringTable.data != nullptr) {
    ec = bela::make_error_code(L"StringTable data not nullptr");
    return false;
  }
  if (fh.PointerToSymbolTable <= 0) {
    // table nullptr
    return true;
  }
  auto offset = fh.PointerToSymbolTable + COFFSymbolSize * fh.NumberOfSymbols;
  uint32_t l = 0;

  if (!ReadAt(&l, sizeof(l), offset, ec)) {
    return false;
  }
  l = bela::swaple(l);
  if (l <= 4) {
    return false;
  }
  l -= 4;
  if (stringTable.data = reinterpret_cast<uint8_t *>(HeapAlloc(GetProcessHeap(), 0, l)); stringTable.data == nullptr) {
    ec = bela::make_system_error_code(L"fail to allocate string table memory: ");
    return false;
  }
  if (!ReadFull(stringTable.data, l, ec)) {
    ec = bela::make_error_code(1, L"fail to read string table: ", ec.message);
    return false;
  }
  stringTable.length = l;
  return true;
}
} // namespace bela::pe