///
#include "internal.hpp"

namespace bela::pe {
// manifest

std::wstring_view ResourceTypeName(ULONG_PTR type) {
  (void)type;
  return L"";
}

} // namespace bela::pe