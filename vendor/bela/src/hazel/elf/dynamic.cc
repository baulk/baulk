//
#include "internal.hpp"

namespace hazel::elf {
bool File::DynString(int tag, std::vector<std::string> &sv, bela::error_code &ec) {
  if (tag != DT_NEEDED && tag != DT_SONAME && tag != DT_RPATH && tag != DT_RUNPATH) {
    ec = bela::make_error_code(1, L"non-string-valued tag ", tag);
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
  std::string_view dss{reinterpret_cast<const char *>(d.data()), d.size()};
  if (fh.Class == ELFCLASS32) {
    while (dss.size() >= 8) {
      auto t = endian_cast_ptr<uint32_t>(dss.data());
      auto v = endian_cast_ptr<uint32_t>(dss.data() + 4);
      dss.remove_prefix(8);
      if (t == tag) {
        if (auto s = getStringO(str.Span(), static_cast<int>(v)); s) {
          sv.emplace_back(std::move(*s));
        }
      }
    }
    return true;
  }
  while (dss.size() >= 16) {
    auto t = endian_cast_ptr<uint64_t>(dss.data());
    auto v = endian_cast_ptr<uint64_t>(dss.data() + 8);
    dss.remove_prefix(16);
    if (t == tag) {
      if (auto s = getStringO(str.Span(), static_cast<int>(v)); s) {
        sv.emplace_back(std::move(*s));
      }
    }
  }
  return true;
}
} // namespace hazel::elf