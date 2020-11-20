///
#include <bela/path.hpp>
#include <bela/env.hpp>
#include "internal.hpp"

namespace bela::pe {

SymbolSearcher::SymbolSearcher(std::wstring_view exe, Machine machine) {
#ifdef _WIN64
  if (machine == Machine::I386 || machine == Machine::ARMNT) {
    Paths.emplace_back(bela::WindowsExpandEnv(L"%SystemRoot%\\SysWOW64"));
  } else {
    Paths.emplace_back(bela::WindowsExpandEnv(L"%SystemRoot%\\System32"));
  }
#else
  if (machine == Machine::I386 || machine == Machine::ARMNT) {
    Paths.emplace_back(bela::WindowsExpandEnv(L"%SystemRoot%\\System32"));
  } else {
    Paths.emplace_back(bela::WindowsExpandEnv(L"%SystemRoot%\\SysNative")); // x86
    Paths.emplace_back(bela::WindowsExpandEnv(L"%SystemRoot%\\System32"));
  }
#endif
  auto self = bela::PathAbsolute(exe);
  bela::PathStripName(self);
  Paths.emplace_back(self);
  auto path_ = bela::GetEnv(L"Path");
  auto paths = bela::SplitPath(path_);
  for (const auto p : paths) {
    Paths.emplace_back(p);
  }
}

std::optional<std::string> SymbolSearcher::LoadOrdinalFunctionName(std::string_view dllname, int ordinal,
                                                                   bela::error_code &ec) {
  auto wdn = bela::ToWide(dllname);
  for (auto &p : Paths) {
    auto file = bela::StringCat(p, L"\\", wdn);
    if (bela::PathExists(file)) {
      if (auto fd = bela::pe::NewFile(file, ec); fd) {
        if (std::vector<bela::pe::ExportedSymbol> es; fd->LookupExports(es, ec)) {
          auto it = table.emplace(dllname, std::move(es));
          if (!it.second) {
            return std::nullopt;
          }
          for (auto &e : it.first->second) {
            if (e.Ordinal == ordinal) {
              return std::make_optional(e.Name);
            }
          }
        }
      }
      return std::nullopt;
    }
  }
  return std::nullopt;
}

std::optional<std::string> SymbolSearcher::LookupOrdinalFunctionName(std::string_view dllname, int ordinal,
                                                                     bela::error_code &ec) {
  if (auto it = table.find(dllname); it != table.end()) {
    for (auto &s : it->second) {
      if (s.Ordinal == ordinal) {
        return std::make_optional(s.Name);
      }
    }
    return std::nullopt;
  }
  return LoadOrdinalFunctionName(dllname, ordinal, ec);
}

} // namespace bela::pe