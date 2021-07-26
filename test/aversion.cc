#include <string_view>
#include <cstdio>
#include <vector>
#include <bela/semver.hpp>

int main() {
  const std::string_view vs[] = {"19.00",
                                 "2020-02-25/2.2.0-24-gb93c271",
                                 "3.2",
                                 "1.35.0",
                                 "v0.13.0",
                                 "2.091.0",
                                 "v8.1.0.0p1-Beta",
                                 "0.49",
                                 "v7.5.0",
                                 "10.0.19041.153",
                                 "7.69.1",
                                 "1.0-rc.1",
                                 "1.2.3-rc.4"};
  for (auto v : vs) {
    bela::version sv(v);
    auto str = sv.make_string_version<char>();
    fprintf(stderr, "resolve version(%u.%u.%u.%u): %s to %s\n", sv.major, sv.minor, sv.patch, sv.build, v.data(),
            str.data());
  }
  return 0;
}
