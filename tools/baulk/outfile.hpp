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
                                        L".rar",
                                        L".tar",
                                        L".tar.gz",
                                        L".tgz",
                                        L".tar.xz",
                                        L".txz",
                                        L".tar.sz",
                                        L".tsz",
                                        L".tar.bz2",
                                        L".tbz2",
                                        L".tar.br",
                                        L".tbr",
                                        L".tar.zst",
                                        L".tzst"};
  for (const auto e : exts) {
    if (bela::EndsWithIgnoreCase(p, e)) {
      return std::wstring(p.substr(0, p.size() - e.size()));
    }
  }
  return bela::StringCat(p, L".out");
}
} // namespace baulk

#endif
