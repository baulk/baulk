#include <bela/process.hpp>
#include <bela/str_split_narrow.hpp>
#include <bela/terminal.hpp>
#include <optional>

std::optional<std::wstring> BucketRepoNewest(std::wstring_view giturl, bela::error_code &ec) {
  bela::process::Process process;
  if (process.Capture(L"git", L"ls-remote", giturl, L"HEAD") != 0) {
    if (process.ExitCode() != 0) {
      ec = bela::make_error_code(process.ExitCode(), L"git exit with: ", process.ExitCode());
    }
    return std::nullopt;
  }
  constexpr std::string_view head = "HEAD";
  std::vector<std::string_view> lines =
      bela::narrow::StrSplit(process.Out(), bela::narrow::ByChar('\n'), bela::narrow::SkipEmpty());
  for (auto line : lines) {
    std::vector<std::string_view> kv =
        bela::narrow::StrSplit(line, bela::narrow::ByAnyChar("\t "), bela::narrow::SkipEmpty());
    if (kv.size() == 2 && kv[1] == head) {
      return std::make_optional(bela::encode_into<char, wchar_t>(kv[0]));
    }
  }
  ec = bela::make_error_code(bela::ErrGeneral, L"not found HEAD: ", giturl);
  return std::nullopt;
}

int wmain(int argc, wchar_t **argv) {
  if (argc < 2) {

    return 1;
  }
  bela::error_code ec;
  auto commit = BucketRepoNewest(argv[1], ec);
  if (!commit) {
    bela::FPrintF(stderr, L"repo head not found: %s\n", ec);
    return 1;
  }
  bela::FPrintF(stderr, L"From %s\n", argv[1]);
  bela::FPrintF(stdout, L"%s\n", *commit);
  return 0;
}