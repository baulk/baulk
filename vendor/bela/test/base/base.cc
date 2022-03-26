#include <bela/base.hpp>
#include <bela/terminal.hpp>
#include <bela/str_cat_narrow.hpp>
#include <bela/win32.hpp>
#include <filesystem>
#include <numeric>

void print() {
  bela::FPrintF(stderr, L"short: %v - %v\n", (std::numeric_limits<short>::min)(),
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
}

int wmain(int argc, wchar_t **argv) {
  print();
  std::filesystem::path p{argv[0]};
  constexpr char32_t sh = 0x1F496; //  💖
  auto xx = 199.9654321f;
  bela::FPrintF(stderr, L"usage: %v [%4c] %.2f\n", p, sh, xx);

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
  bela::FPrintF(stderr, L"%s\n", bela::narrow::StringCat("H: ", bela::narrow::AlphaNum(bela::narrow::Hex(123456))));
  bela::FPrintF(stderr, L"EADDRINUSE: %s\nEWOULDBLOCK: %s\n", bela::make_error_code_from_errno(EADDRINUSE),
                bela::make_error_code_from_errno(EWOULDBLOCK));
  auto version = bela::windows::version();
  bela::FPrintF(stderr, L"%d.%d.%d %d.%d\n", version.major, version.minor, version.build, version.service_pack_major,
                version.service_pack_minor);
  return 0;
}