#include <bela/base.hpp>
#include <bela/terminal.hpp>
#include <bela/str_cat.hpp>
#include <bela/win32.hpp>
#include <bela/strings.hpp>
#include <filesystem>
#include <numeric>
#include <source_location>

void string_no_const_print_v0() {
  char *s1 = nullptr;
  char8_t *s2 = nullptr;
  wchar_t *s3 = nullptr;
  char16_t *s4 = nullptr;
  // char32_t s5[32] = U"中国馆";
  bela::FPrintF(stderr, L"string_no_const_print: %s %s %s %s \n", s1, s2, s3, s4);
}

void string_no_const_print() {
  char s1[32] = "hello world";
  char8_t s2[32] = u8"hello world 2";
  wchar_t s3[32] = L"中国";
  char16_t s4[32] = u"中国";
  char32_t s5[32] = U"🎉💖✔️💜😁😂😊🤣❤️😍😒👌";
  bela::FPrintF(stderr, L"string_no_const_print: %s %s %s %s %s\n", s1, s2, s3, s4, s5);
#ifndef __clang__
  auto current = std::source_location::current();
  bela::FPrintF(stderr, L"%v: %v +%v (%v)\n", current.file_name(), current.function_name(), current.line(),
                current.column());
#endif
}

void string_const_print() {
  constexpr const char *s1 = "hello world";
  constexpr const char8_t *s2 = u8"hello world 2";
  constexpr const wchar_t *s3 = L"中国";
  constexpr const char16_t *s4 = u"中国";
  constexpr const char32_t *s5 = U"🎉💖✔️💜😁😂😊🤣❤️😍😒👌";
  bela::FPrintF(stderr, L"string_const_print: %s %s %s %s %s\n", s1, s2, s3, s4, s5);
}

void string_view_print() {
  std::string_view s1 = "hello world";
  std::u8string_view s2 = u8"hello world 2";
  std::wstring_view s3 = L"中国";
  std::u16string_view s4 = u"中国";
  std::u32string_view s5 = U"🎉💖✔️💜😁😂😊🤣❤️😍😒👌";
  bela::FPrintF(stderr, L"string_view_print: %s %s %s %s %s\n", s1, s2, s3, s4, s5);
}

void string_print() {
  std::string s1 = "hello world";
  std::u8string s2 = u8"hello world 2";
  std::wstring s3 = L"中国";
  std::u16string s4 = u"中国";
  std::u32string s5 = U"🎉💖✔️💜😁😂😊🤣❤️😍😒👌";
  bela::FPrintF(stderr, L"string_print: %s %s %s %s %s\n", s1, s2, s3, s4, s5);
}

// constexpr size_t check_fmt_args(std::wstring_view sv) {
//   size_t count = 0;
//   for (size_t i = 0; i < sv.size(); i++) {
//     auto c = sv[i];
//     if (c != '%' || i + 1 == sv.size()) {
//       continue;
//     }
//     switch (sv[i + 1]) {
//     case '%':
//       i++;
//       break;
//     case 'B':
//       [[fallthrough]];
//     case 'b':
//       [[fallthrough]];
//     case 'c':
//       [[fallthrough]];
//     case 's':
//       [[fallthrough]];
//     case 'd':
//       [[fallthrough]];
//     case 'o':
//       [[fallthrough]];
//     case 'x':
//       [[fallthrough]];
//     case 'X':
//       [[fallthrough]];
//     case 'U':
//       [[fallthrough]];
//     case 'f':
//       [[fallthrough]];
//     case 'a':
//       [[fallthrough]];
//     case 'p':
//       [[fallthrough]];
//     case 'v':
//       count++;
//       i++;
//       break;
//     default:
//       break;
//     }
//   }
//   return count;
// }

// template <class _CharT, class... _Args> struct _Basic_format_string {
//   std::basic_string_view<_CharT> _Str;
//   template <class _Ty>
//   requires std::convertible_to<const _Ty &, std::basic_string_view<_CharT>>
//   consteval _Basic_format_string(const _Ty &_Str_val) : _Str(_Str_val) {
//     static_assert(check_fmt_args(_Str) != sizeof...(Args), "args not equal");
//   }
// };

// template <class... _Args> using _Fmt_string = _Basic_format_string<wchar_t, std::type_identity_t<_Args>...>;

// template <typename... Args> bela::ssize_t FPrintF(FILE *out, const _Fmt_string<Args...> fmt, const Args &...args) {
//   const bela::format_internal::FormatArg arg_array[] = {args...};
//   auto str = bela::format_internal::StrFormatInternal(fmt._Str.data(), arg_array, sizeof...(args));
//   return bela::terminal::WriteAuto(out, str);
// }

void print() {
  // FPrintF(stderr, L"%v ---- %d %s %d", 1, 2, 3);
  // constexpr auto n = check_fmt_args(L"%v %d");
  // constexpr auto n2 = check_fmt_args(L"%v %%");
  // constexpr auto n3 = check_fmt_args(L"---------------");
  bela::FPrintF(stderr, L"short: %Q %v - %v\n", (std::numeric_limits<short>::min)(),
                (std::numeric_limits<short>::max)()); //
  bela::FPrintF(stderr, L"unsigned short: %v - %v\n", (std::numeric_limits<unsigned short>::min)(),
                (std::numeric_limits<unsigned short>::max)()); //
  bela::FPrintF(stderr, L"int: %v - %v\n", (std::numeric_limits<int>::min)(),
                (std::numeric_limits<int>::max)()); //
  bela::FPrintF(stderr, L"unsigned int: %v - %v\n", (std::numeric_limits<unsigned int>::min)(),
                (std::numeric_limits<unsigned int>::max)()); //
  bela::FPrintF(stderr, L"long: %v - %v\n", (std::numeric_limits<long>::min)(),
                (std::numeric_limits<long>::max)()); //
  bela::FPrintF(stderr, L"unsigned long: %v - %v\n", (std::numeric_limits<unsigned long>::min)(),
                (std::numeric_limits<unsigned long>::max)()); //
  bela::FPrintF(stderr, L"long long: %v - %v\n", (std::numeric_limits<long long>::min)(),
                (std::numeric_limits<long long>::max)()); //
  bela::FPrintF(stderr, L"unsigned long long: %v - %v\n", (std::numeric_limits<unsigned long long>::min)(),
                (std::numeric_limits<unsigned long long>::max)()); //
  bela::FPrintF(stderr, L"float: %v - %v\n", (std::numeric_limits<float>::min)(),
                (std::numeric_limits<float>::max)()); //
  bela::FPrintF(stderr, L"double: %v - %v\n", (std::numeric_limits<double>::min)(),
                (std::numeric_limits<double>::max)()); //
  bela::FPrintF(stderr, L"long hex: 0x%08x - 0x%08X\n", (std::numeric_limits<long>::min)(),
                (std::numeric_limits<long>::max)()); //
  printf("printf long hex: %08X - %08X\n", (std::numeric_limits<long>::min)(), (std::numeric_limits<long>::max)());
}

int wmain(int argc, wchar_t **argv) {
  string_print();
  string_view_print();
  string_no_const_print();
  string_const_print();
  string_no_const_print_v0();
  print();
  std::filesystem::path p{argv[0]};
  constexpr char32_t sh = 0x1F496; //  💖
  auto xx = 199.9654321f;
  bela::FPrintF(stderr, L"usage: %v [%4c] %.2f --------------- %j %k --------%W\n", p, sh, xx);
  bela::FPrintF(stderr, L"%v\n", bela::StringCat(L"ls ", p, L" status"));
  if (argc >= 2) {
    FILE *fd = nullptr;

    if (auto e = _wfopen_s(&fd, argv[1], L"rb"); e != 0) {
      auto ec = bela::make_error_code_from_errno(e);
      bela::FPrintF(stderr, L"unable open: %s\n", ec, fd);
      return 1;
    }
    fclose(fd);
  }
  auto ec = bela::make_error_code(bela::ErrCanceled, L"canceled");
  if (ec) {
    bela::FPrintF(stderr, L"Err: %v\n", ec);
  }
  auto ec2 = bela::make_error_code(bela::ErrCanceled, L"cancelede ccc");
  bela::FPrintF(stderr, L"Equal: %v\n", ec == bela::ErrCanceled);
  bela::FPrintF(stderr, L"Equal: %v\n", ec == ec2);
  bela::FPrintF(stderr, L"%s\n",
                bela::StringNarrowCat("H: ", bela::AlphaNumNarrow(bela::Hex(123456, bela::kZeroPad8))));
  bela::FPrintF(stderr, L"EADDRINUSE: %s\nEWOULDBLOCK: %s\n", bela::make_error_code_from_errno(EADDRINUSE),
                bela::make_error_code_from_errno(EWOULDBLOCK));
  auto version = bela::windows::version();
  bela::FPrintF(stderr, L"Windows version: %d.%d.%d %d.%d %v\n", version.major, version.minor, version.build,
                version.service_pack_major, version.service_pack_minor, bela::AlphaNum(true).Piece());
  constexpr auto a = u8"abc";
  constexpr auto b = "abc";
  constexpr auto e = bela::BytesEqual(a, b, 3);
  std::wstring ss;
  bela::StrAppend(&ss, L"kk (", ss.size(), L") ", L"zzz");
  bela::FPrintF(stderr, L"%v\n", ss);
  std::u8string_view text =
      u8"💖 统一码（Unicode），也叫万国码、单一码，是计算机科学领域里的一项业界标准，包括字符集、编"
      u8"码方案等。Unicode 是为了解决传统的字符编码方案的局限而产生的，它为每种语言中的每个字符设"
      u8"定了统一并且唯一的二进制编码，以满足跨语言、跨平台进行文本转换、处理的要求。";
  for (; !text.empty();) {
    auto rune = bela::RuneNext(text);
    if (rune == 0) {
      break;
    }
    bela::FPrintF(stderr, L"%c", rune);
  }
  bela::FPrintF(stderr, L"\n---------------\n");
  std::u16string_view u16text =
      u"💖 统一码（Unicode），也叫万国码、单一码，是计算机科学领域里的一项业界标准，包括字符集、编"
      u"码方案等。Unicode 是为了解决传统的字符编码方案的局限而产生的，它为每种语言中的每个字符设"
      u"定了统一并且唯一的二进制编码，以满足跨语言、跨平台进行文本转换、处理的要求。";
  for (; !u16text.empty();) {
    auto rune = bela::RuneNext(u16text);
    if (rune == 0) {
      break;
    }
    bela::FPrintF(stderr, L"%c", rune);
  }
  std::wstring abc;
  abc.push_back('\0');
  abc.append(L"\0");
  bela::FPrintF(stderr, L"\n---------------: %d\n", abc.size());
  return 0;
}