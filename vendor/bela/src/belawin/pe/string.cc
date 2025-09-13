///
#include <utility>

#include "internal.hpp"

namespace bela::pe {

bool File::readStringTable(bela::error_code &ec) {
  if (fh.PointerToSymbolTable == 0) {
    // table nullptr
    return true;
  }
  auto offset = fh.PointerToSymbolTable + (COFFSymbolSize * fh.NumberOfSymbols);
  if (std::cmp_greater_equal(offset + 4, size)) {
    return true;
  }
  uint32_t l = 0;
  if (!fd.ReadAt(l, offset, ec)) {
    return false;
  }
  l = bela::fromle(l);
  if (l <= 4 || std::cmp_greater(l + offset, size)) {
    return true;
  }
  l -= 4;
  bela::Buffer b(l);
  if (!fd.ReadFull(b, l, ec)) {
    ec = bela::make_error_code(ErrGeneral, L"fail to read string table: ", ec.message);
    return false;
  }
  stringTable.buffer = std::move(b);
  return true;
}
} // namespace bela::pe