#ifndef BAULK_OUTFILE_HPP
#define BAULK_OUTFILE_HPP
#include <bela/match.hpp>
#include <bela/strcat.hpp>

namespace baulk {
inline std::wstring ArchiveExtensionConversion(std::wstring_view p) {
  constexpr std::wstring_view exts[] = {L".zip",
                                        L".7z",
                                        L".exe",
                                        L".msi",
                                        L".rar"
                                        L".tar",
                                        L".tar.gz",
                                        L".tar.xz",
                                        L".tar.sz",
                                        L".tar.bz",
                                        L".tar.br",
                                        L".tar.zst"};
  for (const auto e : exts) {
    if (bela::EndsWithIgnoreCase(p, e)) {
      return std::wstring(p.substr(0, p.size() - e.size()));
    }
  }
  return bela::StringCat(p, L".out");
}
} // namespace baulk

#endif