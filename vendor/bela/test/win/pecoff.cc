///
#include <bela/pe.hpp>
#include <bela/terminal.hpp>
#include <bela/und.hpp>

int wmain(int argc, wchar_t **argv) {
  if (argc < 2) {
    bela::FPrintF(stderr, L"usage: %s pefile\n", argv[0]);
    return 1;
  }
  bela::error_code ec;
  bela::pe::File file;
  if (!file.NewFile(argv[1], ec)) {
    bela::FPrintF(stderr, L"unable parse pecoff: %s\n", ec.message);
    return 1;
  }
  bela::FPrintF(stdout,
                L"Is64Bit: %b\nMachine: %d\nCharacteristics: %d\nPointerToSymbolTable: %d\nNumberOfSymbols %d\n",
                file.Is64Bit(), file.Fh().Machine, file.Fh().Characteristics, file.Fh().PointerToSymbolTable,
                file.Fh().NumberOfSymbols);
  std::vector<std::string_view> sa;
  file.SplitStringTable(sa);
  for (const auto s : sa) {
    bela::FPrintF(stdout, L"%s\n", s);
  }
  if (file.Is64Bit()) {
    bela::FPrintF(stdout, L"Subsystem %d\n", file.Oh64()->Subsystem);
  }
  for (const auto &sec : file.Sections()) {
    bela::FPrintF(stdout, L"Section: %s VirtualAddress: %d\n", sec.Header.Name, sec.Header.VirtualAddress);
  }
  bela::pe::SymbolSearcher sse(argv[1], file.Machine());
  bela::pe::FunctionTable ft;
  file.LookupFunctionTable(ft, ec);
  for (const auto &d : ft.imports) {
    bela::FPrintF(stdout, L"\x1b[33mDllName: %s\x1b[0m\n", d.first);
    for (const auto &n : d.second) {
      if (n.Ordinal == 0) {
        bela::FPrintF(stdout, L"%s %d\n", llvm::demangle(n.Name), n.Index);

        continue;
      }
      if (auto fn = sse.LookupOrdinalFunctionName(d.first, n.Ordinal, ec); fn) {
        bela::FPrintF(stdout, L"%s (Ordinal %d)\n", llvm::demangle(*fn), n.Ordinal);
        continue;
      }
      bela::FPrintF(stdout, L"Ordinal%d (Ordinal %d)\n", n.Ordinal, n.Ordinal);
    }
  }

  for (const auto &d : ft.delayimprots) {
    bela::FPrintF(stdout, L"\x1b[34mDllName: %s\x1b[0m\n", d.first);
    for (const auto &n : d.second) {
      if (n.Ordinal == 0) {
        bela::FPrintF(stdout, L"(Delay) %s %d\n", n.Name, n.Index);
        continue;
      }
      if (auto fn = sse.LookupOrdinalFunctionName(d.first, n.Ordinal, ec); fn) {
        bela::FPrintF(stdout, L"(Delay) %s (Ordinal %d)\n", bela::demangle(*fn), n.Ordinal);
        continue;
      }
      bela::FPrintF(stdout, L"(Delay) Ordinal%d (Ordinal %d)\n", n.Ordinal, n.Ordinal);
    }
  }
  // amd64_microsoft.windows.common-controls_6595b64144ccf1df_6.0.19041.488_none_ca04af081b815d21
  for (const auto &d : ft.exports) {
    bela::FPrintF(stdout, L"\x1b[35mE %5d %08X %s  (Hint: %d)\x1b[0m\n", d.Ordinal, d.Address, llvm::demangle(d.Name),
                  d.Hint);
  }
  std::vector<bela::pe::Symbol> syms;
  if (file.LookupSymbols(syms, ec)) {
    for (const auto &sm : syms) {
      bela::FPrintF(stdout, L"Symbols: %08x %s\n", sm.Value, sm.Name);
    }
  }
  std::string ver;
  if (file.LookupClrVersion(ver, ec) && !ver.empty()) {
    bela::FPrintF(stdout, L"CRL: %s\n", ver);
  }
  std::vector<char> overlay;
  if (!file.LookupOverlay(overlay, ec) && ec.code != bela::pe::ErrNoOverlay) {
    bela::FPrintF(stderr, L"Overlay: %s\n", ec.message);
    return 1;
  }
  bela::FPrintF(stderr, L"Overlay: %s\n", std::string_view{overlay.data(), overlay.size()});
  return 0;
}