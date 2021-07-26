///
#include <bela/pe.hpp>
#include <bela/terminal.hpp>
#include <bela/und.hpp>

static const constexpr char hex[] = "0123456789abcdef";

static int color(uint8_t b) {
  constexpr unsigned char CN = 0x37; /* null    */
  constexpr unsigned char CS = 0x92; /* space   */
  constexpr unsigned char CP = 0x96; /* print   */
  constexpr unsigned char CC = 0x95; /* control */
  constexpr unsigned char CH = 0x93; /* high    */
  static constexpr const unsigned char table[] = {
      CN, CC, CC, CC, CC, CC, CC, CC, CC, CC, CS, CS, CS, CS, CC, CC, CC, CC, CC, CC, CC, CC, CC, CC, CC, CC,
      CC, CC, CC, CC, CC, CC, CS, CP, CP, CP, CP, CP, CP, CP, CP, CP, CP, CP, CP, CP, CP, CP, CP, CP, CP, CP,
      CP, CP, CP, CP, CP, CP, CP, CP, CP, CP, CP, CP, CP, CP, CP, CP, CP, CP, CP, CP, CP, CP, CP, CP, CP, CP,
      CP, CP, CP, CP, CP, CP, CP, CP, CP, CP, CP, CP, CP, CP, CP, CP, CP, CP, CP, CP, CP, CP, CP, CP, CP, CP,
      CP, CP, CP, CP, CP, CP, CP, CP, CP, CP, CP, CP, CP, CP, CP, CP, CP, CP, CP, CP, CP, CP, CP, CC, CH, CH,
      CH, CH, CH, CH, CH, CH, CH, CH, CH, CH, CH, CH, CH, CH, CH, CH, CH, CH, CH, CH, CH, CH, CH, CH, CH, CH,
      CH, CH, CH, CH, CH, CH, CH, CH, CH, CH, CH, CH, CH, CH, CH, CH, CH, CH, CH, CH, CH, CH, CH, CH, CH, CH,
      CH, CH, CH, CH, CH, CH, CH, CH, CH, CH, CH, CH, CH, CH, CH, CH, CH, CH, CH, CH, CH, CH, CH, CH, CH, CH,
      CH, CH, CH, CH, CH, CH, CH, CH, CH, CH, CH, CH, CH, CH, CH, CH, CH, CH, CH, CH, CH, CH, CH, CH, CH, CH,
      CH, CH, CH, CH, CH, CH, CH, CH, CH, CH, CH, CH, CH, CH, CH, CH, CH, CH, CH, CH, CH, CH};
  return table[b];
}

static int display(uint8_t b) {
  static constexpr const char table[] = {
      0x2e, 0x2e, 0x2e, 0x2e, 0x2e, 0x2e, 0x2e, 0x2e, 0x2e, 0x2e, 0x2e, 0x2e, 0x2e, 0x2e, 0x2e, 0x2e, 0x2e, 0x2e, 0x2e,
      0x2e, 0x2e, 0x2e, 0x2e, 0x2e, 0x2e, 0x2e, 0x2e, 0x2e, 0x2e, 0x2e, 0x2e, 0x2e, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25,
      0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38,
      0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f, 0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4a, 0x4b,
      0x4c, 0x4d, 0x4e, 0x4f, 0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5a, 0x5b, 0x5c, 0x5d, 0x5e,
      0x5f, 0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f, 0x70, 0x71,
      0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7a, 0x7b, 0x7c, 0x7d, 0x7e, 0x2e, 0x2e, 0x2e, 0x2e, 0x2e, 0x2e,
      0x2e, 0x2e, 0x2e, 0x2e, 0x2e, 0x2e, 0x2e, 0x2e, 0x2e, 0x2e, 0x2e, 0x2e, 0x2e, 0x2e, 0x2e, 0x2e, 0x2e, 0x2e, 0x2e,
      0x2e, 0x2e, 0x2e, 0x2e, 0x2e, 0x2e, 0x2e, 0x2e, 0x2e, 0x2e, 0x2e, 0x2e, 0x2e, 0x2e, 0x2e, 0x2e, 0x2e, 0x2e, 0x2e,
      0x2e, 0x2e, 0x2e, 0x2e, 0x2e, 0x2e, 0x2e, 0x2e, 0x2e, 0x2e, 0x2e, 0x2e, 0x2e, 0x2e, 0x2e, 0x2e, 0x2e, 0x2e, 0x2e,
      0x2e, 0x2e, 0x2e, 0x2e, 0x2e, 0x2e, 0x2e, 0x2e, 0x2e, 0x2e, 0x2e, 0x2e, 0x2e, 0x2e, 0x2e, 0x2e, 0x2e, 0x2e, 0x2e,
      0x2e, 0x2e, 0x2e, 0x2e, 0x2e, 0x2e, 0x2e, 0x2e, 0x2e, 0x2e, 0x2e, 0x2e, 0x2e, 0x2e, 0x2e, 0x2e, 0x2e, 0x2e, 0x2e,
      0x2e, 0x2e, 0x2e, 0x2e, 0x2e, 0x2e, 0x2e, 0x2e, 0x2e, 0x2e, 0x2e, 0x2e, 0x2e, 0x2e, 0x2e, 0x2e, 0x2e, 0x2e, 0x2e,
      0x2e, 0x2e, 0x2e, 0x2e, 0x2e, 0x2e, 0x2e, 0x2e, 0x2e,
  };
  return table[b];
}

static void process_color(FILE *out, std::string_view sv, int64_t offset) {
  size_t i, n;
  unsigned char input[16] = {0};
  constexpr const uint64_t inputlen = sizeof(input);
  char colortemplate[] = "00000000  "
                         "\33[XXm## \33[XXm## \33[XXm## \33[XXm## "
                         "\33[XXm## \33[XXm## \33[XXm## \33[XXm##  "
                         "\33[XXm## \33[XXm## \33[XXm## \33[XXm## "
                         "\33[XXm## \33[XXm## \33[XXm## \33[XXm##  "
                         "\33[XXm.\33[XXm.\33[XXm.\33[XXm.\33[XXm.\33[XXm.\33[XXm.\33[XXm."
                         "\33[XXm.\33[XXm.\33[XXm.\33[XXm.\33[XXm.\33[XXm.\33[XXm.\33[XXm."
                         "\33[0m\n";
  static const int slots[] = {/* ANSI-color, hex, ANSI-color, ASCII */
                              12,  15,  142, 145, 20,  23,  148, 151, 28,  31,  154, 157, 36,  39,  160, 163,
                              44,  47,  166, 169, 52,  55,  172, 175, 60,  63,  178, 181, 68,  71,  184, 187,
                              77,  80,  190, 193, 85,  88,  196, 199, 93,  96,  202, 205, 101, 104, 208, 211,
                              109, 112, 214, 217, 117, 120, 220, 223, 125, 128, 226, 229, 133, 136, 232, 235};
  uint64_t maxlen = sv.size();
  do {
    n = (std::min)(maxlen, inputlen);
    memcpy(input, sv.data(), static_cast<size_t>(n));
    sv.remove_prefix(n);
    maxlen -= n;
    /* Write the offset */
    for (i = 0; i < 8; i++) {
      colortemplate[i] = hex[(offset >> (28 - i * 4)) & 15];
    }

    /* Fill out the colortemplate */
    for (i = 0; i < 16; i++) {
      /* Use a fixed loop count instead of "n" to encourage loop
       * unrolling by the compiler. Empty bytes will be erased
       * later.
       */
      int v = input[i];
      int c = color(v);
      colortemplate[slots[i * 4 + 0] + 0] = hex[c >> 4];
      colortemplate[slots[i * 4 + 0] + 1] = hex[c & 15];
      colortemplate[slots[i * 4 + 1] + 0] = hex[v >> 4];
      colortemplate[slots[i * 4 + 1] + 1] = hex[v & 15];
      colortemplate[slots[i * 4 + 2] + 0] = hex[c >> 4];
      colortemplate[slots[i * 4 + 2] + 1] = hex[c & 15];
      colortemplate[slots[i * 4 + 3] + 0] = display(v);
    }

    /* Erase any trailing bytes */
    for (i = n; i < 16; i++) {
      /* This loop is only used once: the last line of output. The
       * branch predictor will quickly learn that it's never taken.
       */
      colortemplate[slots[i * 4 + 0] + 0] = '0';
      colortemplate[slots[i * 4 + 0] + 1] = '0';
      colortemplate[slots[i * 4 + 1] + 0] = ' ';
      colortemplate[slots[i * 4 + 1] + 1] = ' ';
      colortemplate[slots[i * 4 + 2] + 0] = '0';
      colortemplate[slots[i * 4 + 2] + 1] = '0';
      colortemplate[slots[i * 4 + 3] + 0] = ' ';
    }

    if (!bela::terminal::WriteAuto(out, std::string_view{colortemplate, sizeof(colortemplate) - 1})) {
      break; /* Output error */
    }
    offset += 16;
  } while (n == 16 && maxlen > 0);
}

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
    bela::FPrintF(stdout, L"Subsystem %d\n", static_cast<int>(file.Subsystem()));
  }
  for (const auto &sec : file.Sections()) {
    bela::FPrintF(stdout, L"Section: %s VirtualAddress: %d\n", sec.Name, sec.VirtualAddress);
  }
  bela::pe::SymbolSearcher sse(argv[1], file.Machine());
  bela::pe::FunctionTable ft;
  file.LookupFunctionTable(ft, ec);
  for (const auto &d : ft.imports) {
    bela::FPrintF(stdout, L"\x1b[33mDllName: %s\x1b[0m\n", d.first);
    for (const auto &n : d.second) {
      if (n.Ordinal == 0) {
        bela::FPrintF(stdout, L" %7d %s\n", n.Index, llvm::demangle(n.Name));
        continue;
      }
      if (auto fn = sse.LookupOrdinalFunctionName(d.first, n.Ordinal, ec); fn) {
        bela::FPrintF(stdout, L"         %s (Ordinal %d)\n", llvm::demangle(*fn), n.Ordinal);
        continue;
      }
      bela::FPrintF(stdout, L"         Ordinal%d (Ordinal %d)\n", n.Ordinal, n.Ordinal);
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
  if (auto dm = file.LookupDotNetMetadata(ec); dm) {
    bela::FPrintF(stdout, L"CRL Version: %s\n", dm->version);
    bela::FPrintF(stdout, L"Flags: %s\n", dm->flags);
  }
  auto overlayLen = file.OverlayLength();
  bela::FPrintF(stderr, L"Overlay offset 0x%08x, length: %d\n", file.OverlayOffset(), file.OverlayLength());
  if (overlayLen == 0) {
    return 0;
  }
  uint8_t buffer[2048];
  auto ret = file.ReadOverlay(buffer, ec);
  if (ret < 0) {
    bela::FPrintF(stderr, L"Error: %s\n", ec.message);
    return 1;
  }
  bela::FPrintF(stderr, L"Read %d bytes \n", ret);
  process_color(stderr, std::string_view{reinterpret_cast<char *>(buffer), static_cast<size_t>(ret)},
                file.OverlayOffset());
  return 0;
}