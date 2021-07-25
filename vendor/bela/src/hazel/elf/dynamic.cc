//
#include <hazel/elf.hpp>

namespace hazel::elf {
bool File::DynString(int tag, std::vector<std::string> &sv, bela::error_code &ec) const {
  if (tag != DT_NEEDED && tag != DT_SONAME && tag != DT_RPATH && tag != DT_RUNPATH) {
    ec = bela::make_error_code(ErrGeneral, L"non-string-valued tag ", tag);
    return false;
  }
  auto ds = SectionByType(SHT_DYNAMIC);
  if (ds == nullptr) {
    return true;
  }
  bela::Buffer d;
  if (!sectionData(*ds, d, ec)) {
    return false;
  }
  bela::Buffer str;
  if (!stringTable(ds->Link, str, ec)) {
    return false;
  }
  auto bsv = str.as_bytes_view();
  auto dv = d.as_bytes_view();
  if (fh.Class == ELFCLASS32) {
    while (dv.size() >= 8) {
      auto t = cast_from<uint32_t>(dv.data());
      auto v = cast_from<uint32_t>(dv.data() + 4);
      dv.remove_prefix(8);
      if (static_cast<int>(t) == tag) {
        if (auto s = bsv.make_cstring_view(v); !s.empty()) {
          sv.emplace_back(s);
        }
      }
    }
    return true;
  }
  while (dv.size() >= 16) {
    auto t = cast_from<uint64_t>(dv.data());
    auto v = cast_from<uint64_t>(dv.data() + 8);
    dv.remove_prefix(16);
    if (static_cast<int>(t) == tag) {
      if (auto s = bsv.make_cstring_view(v); !s.empty()) {
        sv.emplace_back(s);
      }
    }
  }
  return true;
}
} // namespace hazel::elf