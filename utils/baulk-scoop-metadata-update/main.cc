#include <bela/base.hpp>
#include <bela/terminal.hpp>
#include <baulk/json_utils.hpp>
#include <bela/semver.hpp>

int convert_from(std::wstring_view theirs, std::wstring_view ours) {
  bela::error_code ec;
  auto tv = baulk::json::parse_file(theirs, ec);
  if (!tv) {
    bela::FPrintF(stderr, L"parse scoop json file: %v\n", ec);
    return 1;
  }
  auto ov = baulk::json::parse_file(ours, ec);
  if (!ov) {
    bela::FPrintF(stderr, L"parse baulk json file: %v\n", ec);
    return 1;
  }

  return 0;
}

int wmain(int argc, wchar_t **argv) {
  if (argc < 3) {
    bela::FPrintF(stderr, L"usage: %v scoop-json-file baulk-json-file\n", argv[0]);
    return 1;
  }
  return 0;
}