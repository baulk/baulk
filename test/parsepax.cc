///
#include <bela/base.hpp>
#include <bela/terminal.hpp>
#include <charconv>

bool parsePAXRecord(std::string_view *sv, std::string_view *k, std::string_view *v, bela::error_code &ec) {
  auto pos = sv->find(' ');
  if (pos == std::string_view::npos) {
    ec = bela::make_error_code(L"invalid tar header");
    return false;
  }
  int n = 0;
  if (auto res = std::from_chars(sv->data(), sv->data() + pos, n); res.ec != std::errc{} || n > sv->size()) {
    ec = bela::make_error_code(bela::ErrGeneral, L"invalid number '", bela::encode_into<char, wchar_t>(sv->substr(0, pos)), L"'");
    return false;
  }
  auto rec = sv->substr(pos + 1, n - pos - 1);
  if (!rec.ends_with('\n')) {
    ec = bela::make_error_code(L"invalid tar header");
    return false;
  }
  rec.remove_suffix(1);
  pos = rec.find('=');
  if (pos == std::string_view::npos) {
    ec = bela::make_error_code(L"invalid tar header");
    return false;
  }
  *k = rec.substr(0, pos);
  *v = rec.substr(pos + 1);
  sv->remove_prefix(n);
  return true;
}

int wmain() {
  std::string_view pax = "17 helloworld=xx\n19 path=vvvvvvvvvv\n";
  bela::error_code ec;
  while (!pax.empty()) {
    std::string_view k;
    std::string_view v;
    if (!parsePAXRecord(&pax, &k, &v, ec)) {
      bela::FPrintF(stderr, L"Error %s\n", ec.message);
      return 1;
    }
    bela::FPrintF(stderr, L"K %s = V %s\n", k, v);
  }
  return 0;
}