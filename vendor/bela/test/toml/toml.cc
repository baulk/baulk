#include <bela/terminal.hpp>
#include <bela/toml.hpp>

std::vector<std::string_view> split(std::string_view value, char separator) {
  std::vector<std::string_view> result;
  size_t first = 0;
  while (first < value.size()) {
    const auto second = value.find_first_of(separator, first);
    auto s = value.substr(first, second - first);
    result.emplace_back(s);
    if (second == std::string_view::npos) {
      break;
    }
    first = second + 1;
  }
  return result;
}

void test_split() {
  constexpr std::string_view svv[] = {"test.data.some", "ok"};
  for (auto s : svv) {
    auto sv = split(s, '.');
    bela::FPrintF(stderr, L"TO %d\n[", sv.size());
    for (auto a : sv) {
      bela::FPrintF(stderr, L" '%s' ", a);
    }
    bela::FPrintF(stderr, L"]\n");
  }
}

int wmain(int argc, wchar_t **argv) {
  if (argc < 2) {
    bela::FPrintF(stderr, L"usage: %s toml file\n", argv[0]);
    return 1;
  }
  test_split();
  try {
    auto t = bela::toml::parse_file(argv[1]);
    // if (t->contains_qualified("Title.Last")) {
    //   //
    // }
    auto i = t->get_qualified_as<int64_t>("App.RedisPort");
    if (i) {
      bela::FPrintF(stderr, L"redis port is %d\n", *i);
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