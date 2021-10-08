#include <bela/ascii.hpp>
#include <bela/terminal.hpp>

void parseLine1(std::wstring_view sv) {
  bela::FPrintF(stderr, L"\x1b[33m---> std::wstring_view        [%s]\x1b[0m\n", sv);
  bela::FPrintF(stderr, L"StripLeadingAsciiWhitespace:  [%s]\n", bela::StripLeadingAsciiWhitespace(sv));
  bela::FPrintF(stderr, L"StripTrailingAsciiWhitespace: [%s]\n", bela::StripTrailingAsciiWhitespace(sv));
  bela::FPrintF(stderr, L"StripAsciiWhitespace:         [%s]\n", bela::StripAsciiWhitespace(sv));
  bela::FPrintF(stderr, L"AsciiStrToLower:              [%s]\n", bela::AsciiStrToLower(sv));
  bela::FPrintF(stderr, L"AsciiStrToUpper:              [%s]\n", bela::AsciiStrToUpper(sv));
}

void parseLine2(std::wstring sv) {
  bela::FPrintF(stderr, L"\x1b[34mstd::wstring                  [%s]\x1b[0m\n", sv);
  std::wstring s1(sv);
  bela::StripLeadingAsciiWhitespace(&s1);
  bela::FPrintF(stderr, L"StripLeadingAsciiWhitespace:  [%s]\n", s1);
  std::wstring s2(sv);
  bela::StripTrailingAsciiWhitespace(&s2);
  bela::FPrintF(stderr, L"StripTrailingAsciiWhitespace: [%s]\n", s2);
  std::wstring s3(sv);
  bela::StripAsciiWhitespace(&s3);
  bela::FPrintF(stderr, L"StripAsciiWhitespace:         [%s]\n", s3);
  std::wstring s4(sv);
  bela::RemoveExtraAsciiWhitespace(&s4);
  bela::FPrintF(stderr, L"RemoveExtraAsciiWhitespace:   [%s]\n", s4);
  bela::FPrintF(stderr, L"AsciiStrToLower:              [%s]\n", bela::AsciiStrToLower(sv));
  bela::FPrintF(stderr, L"AsciiStrToUpper:              [%s]\n", bela::AsciiStrToUpper(sv));
}

void parseLine3(std::string_view sv) {
  bela::FPrintF(stderr, L"\x1b[35m---> std::string_view         [%s]\x1b[0m\n", sv);
  bela::FPrintF(stderr, L"StripLeadingAsciiWhitespace:  [%s]\n", bela::StripLeadingAsciiWhitespace(sv));
  bela::FPrintF(stderr, L"StripTrailingAsciiWhitespace: [%s]\n", bela::StripTrailingAsciiWhitespace(sv));
  bela::FPrintF(stderr, L"StripAsciiWhitespace:         [%s]\n", bela::StripAsciiWhitespace(sv));
  bela::FPrintF(stderr, L"AsciiStrToLower:              [%s]\n", bela::AsciiStrToLower(sv));
  bela::FPrintF(stderr, L"AsciiStrToUpper:              [%s]\n", bela::AsciiStrToUpper(sv));
}

void parseLine4(std::string sv) {
  bela::FPrintF(stderr, L"\x1b[36m--->std::string               [%s]\x1b[0m\n", sv);
  std::string s1(sv);
  bela::StripLeadingAsciiWhitespace(&s1);
  bela::FPrintF(stderr, L"StripLeadingAsciiWhitespace:  [%s]\n", s1);
  std::string s2(sv);
  bela::StripTrailingAsciiWhitespace(&s2);
  bela::FPrintF(stderr, L"StripTrailingAsciiWhitespace: [%s]\n", s2);
  std::string s3(sv);
  bela::StripAsciiWhitespace(&s3);
  bela::FPrintF(stderr, L"StripAsciiWhitespace:         [%s]\n", s3);
  std::string s4(sv);
  bela::RemoveExtraAsciiWhitespace(&s4);
  bela::FPrintF(stderr, L"RemoveExtraAsciiWhitespace:   [%s]\n", s4);
  bela::FPrintF(stderr, L"AsciiStrToLower:              [%s]\n", bela::AsciiStrToLower(sv));
  bela::FPrintF(stderr, L"AsciiStrToUpper:              [%s]\n", bela::AsciiStrToUpper(sv));
}

int wmain() {
  constexpr const wchar_t *sv[] = {
      L"ABCABC---- ", L"abcabc    ABC ABC", L"\r\nABC   ",       L" Whitespace \t  in\v   middle  ",
      L"ABC ABC",     L"IKKKKKK   ",        L"long time ago   ",
  };
  for (const auto s : sv) {
    parseLine1(s);
    parseLine2(s);
    auto ns = bela::encode_into<wchar_t, char>(s);
    parseLine3(ns);
    parseLine4(ns);
  }
  return 0;
}