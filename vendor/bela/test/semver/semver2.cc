#include <bela/semver.hpp>
#include <bela/terminal.hpp>

int main() {
  bela::semver::version v1(L"7.8.1-rc.1");
  bela::semver::version v2("7.8.1-rc.1");
  if (v1 == v2) {
    fprintf(stderr, "v1(%s)==v2(%s)\n", v1.to_string().data(), v2.to_string().data());
    return 0;
  }
  fprintf(stderr, "v1(%s)!=v2(%s)\n", v1.to_string().data(), v2.to_string().data());
  return 1;
}
