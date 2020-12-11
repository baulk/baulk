///
#include <bela/path.hpp>
#include <bela/env.hpp>
#include "internal.hpp"

namespace bela::pe {

#ifdef _WIN64
constexpr bool IsWow64 = false;
#else
// 919, api-ms-win-core-wow64-l1-1-1.IsWow64Process2, IsWow64Process2, 919, 918
typedef BOOL(WINAPI *IsWow64Process2)(HANDLE hProcess, USHORT *pProcessMachine, USHORT *pNativeMachine);
bool BelaIsWow64Process() {
  auto hmod = GetModuleHandleW(L"kernel32.dll");
  auto isWow64Process2 = reinterpret_cast<IsWow64Process2>(GetProcAddress(hmod, "IsWow64Process2"));
  uint16_t pm = 0;
  uint16_t nm = 0;
  if (isWow64Process2 && isWow64Process2(GetCurrentProcess(), &pm, &nm)) {
    return nm == IMAGE_FILE_MACHINE_ARM64 || nm == IMAGE_FILE_MACHINE_AMD64;
  }
  return false;
}
static bool IsWow64 = BelaIsWow64Process();
#endif

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
      bela::pe::File fd;
      if (!fd.NewFile(file, ec)) {
        continue;
      }
      if (std::vector<bela::pe::ExportedSymbol> es; fd.LookupExports(es, ec)) {
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