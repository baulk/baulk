#include <bela/stdwriter.hpp>
#include <bela/finaly.hpp>
#include "commands.hpp"
#include "fs.hpp"
#include "baulk.hpp"
#include "jsonex.hpp"
#include "io.hpp"

namespace baulk::commands {

inline bool BaulkLoad(nlohmann::json &json, bela::error_code &ec) {
  FILE *fd = nullptr;
  if (auto eo = _wfopen_s(&fd, baulk::BaulkProfile().data(), L"rb"); eo != 0) {
    ec = bela::make_stdc_error_code(eo);
    return false;
  }
  auto closer = bela::finally([&] { fclose(fd); });
  try {
    json = nlohmann::json::parse(fd);
  } catch (const std::exception &e) {
    ec = bela::make_error_code(1, bela::ToWide(e.what()));
    return false;
  }
  return true;
}

inline bool BaulkStore(std::string_view jsontext, bela::error_code &ec) {
  return baulk::io::WriteTextAtomic(jsontext, baulk::BaulkProfile(), ec);
}

int cmd_freeze(const argv_t &argv) {
  if (argv.empty()) {
    bela::FPrintF(stderr, L"usage: baulk freeze package ...\n");
    return 1;
  }
  nlohmann::json json;
  bela::error_code ec;
  if (!BaulkLoad(json, ec)) {
    bela::FPrintF(stderr, L"unable load baulk profile: %s\n", ec.message);
    return 1;
  }
  std::vector<std::string> pkgs;
  for (auto p : argv) {
    pkgs.emplace_back(bela::ToNarrow(p));
  }
  auto contains = [&](std::string_view p_) {
    for (const auto &p : pkgs) {
      if (p == p_) {
        return true;
      }
    }
    return false;
  };
  try {
    auto it = json.find("freeze");
    if (it != json.end()) {
      for (const auto &freeze : it.value()) {
        auto p = freeze.get<std::string_view>();
        if (!contains(p)) {
          pkgs.emplace_back(p);
        }
      }
    }
    json["freeze"] = pkgs;
    auto jsontext = json.dump(4);
    if (!BaulkStore(jsontext, ec)) {
      bela::FPrintF(stderr, L"unable store baulk freeze package: %s\n",
                    ec.message);
      return 1;
    }
  } catch (const std::exception &e) {
    bela::FPrintF(stderr, L"unable store freeze package: %s\n", e.what());
    return 1;
  }
  bela::FPrintF(stderr, L"baulk freeze package success, freezed: %d\n",
                pkgs.size());
  return 0;
}
int cmd_unfreeze(const argv_t &argv) {
  if (argv.empty()) {
    bela::FPrintF(stderr, L"usage: baulk unfreeze package ...\n");
    return 1;
  }
  nlohmann::json json;
  bela::error_code ec;
  if (!BaulkLoad(json, ec)) {
    bela::FPrintF(stderr, L"unable load baulk profile: %s\n", ec.message);
    return 1;
  }
  auto contains = [&](std::string_view p) {
    auto w = bela::ToWide(p);
    for (auto a : argv) {
      if (a == w) {
        return true;
      }
    }
    return false;
  };
  std::vector<std::string> pkgs;
  try {
    auto it = json.find("freeze");
    if (it != json.end()) {
      for (const auto &freeze : it.value()) {
        auto p = freeze.get<std::string_view>();
        if (!contains(p)) {
          pkgs.emplace_back(p);
        }
      }
    }
    json["freeze"] = pkgs;
    auto jsontext = json.dump(4);
    if (!BaulkStore(jsontext, ec)) {
      bela::FPrintF(stderr, L"unable store baulk profile: %s\n", ec.message);
      return 1;
    }
  } catch (const std::exception &e) {
    bela::FPrintF(stderr, L"unable store baulk profile: %s\n", e.what());
    return 1;
  }
  bela::FPrintF(stderr, L"baulk unfreeze package success, freezed: %d\n",
                pkgs.size());
  return 0;
}

} // namespace baulk::commands