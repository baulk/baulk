#include "baulk.hpp"
#include "commands.hpp"

namespace baulk::commands {
struct PackageSimple {
  void Append(std::string_view name, std::string_view version,
              std::string_view description) {
    auto nw = bela::StringWidth(name);
    namewidth.emplace_back(nw);
    auto vw = bela::StringWidth(version);
    versionwidth.emplace_back(vw);
    namemax = (std::max)(nw, namemax);
    versionmax = (std::max)(vw, versionmax);
    names.emplace_back(bela::ToWide(name));
    versions.emplace_back(bela::ToWide(version));
    descriptions.emplace_back(bela::ToWide(description));
  }
  std::wstring Pretty() {
    std::wstring space;
    space.resize((std::max)(versionmax, namemax) + 2, L' ');
    std::wstring s;
    for (size_t i = 0; i < names.size(); i++) {
      bela::StrAppend(&s, names[i], space.substr(0, namemax + 2 - namewidth[i]),
                      versions[i],
                      space.substr(0, versionmax + 2 - versionwidth[i]), L"\n");
    }
    return s;
  }
  std::vector<std::wstring> names;
  std::vector<std::wstring> versions;
  std::vector<std::wstring> descriptions;
  std::vector<size_t> namewidth;
  std::vector<size_t> versionwidth;
  size_t namemax{0};
  size_t versionmax{0};
};
int cmd_search_all() {
  //
  return 0;
}

int cmd_search(const argv_t &argv) {
  if (!baulk::InitializeBaulkBuckets()) {
    return 1;
  }
  //
  if (argv.empty()) {
    return cmd_search_all();
  }
  for (const auto pkg : argv) {
    //
  }
  return 0;
}
} // namespace baulk::commands