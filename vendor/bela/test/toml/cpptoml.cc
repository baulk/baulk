#include <bela/terminal.hpp>
#include <bela/codecvt.hpp>
#define CPPTOML_NO_RTTI
#include "cpptoml.h"

int wmain(int argc, wchar_t **argv) {
  if (argc < 2) {
    bela::FPrintF(stderr, L"usage: %s toml file\n", argv[0]);
    return 1;
  }
  try {
    auto t = cpptoml::parse_file(bela::ToNarrow(argv[1]));
    if (t->contains_qualified("Title.Last")) {
      //
    }
    auto v = t->get_qualified_as<std::string>("Title.Last");
    if (v) {
      bela::FPrintF(stderr, L"value: %s\n", *v);
    }
  } catch (const std::exception &e) {
    bela::FPrintF(stderr, L"Exception: %s\n", e.what());
  }
  return 0;
}