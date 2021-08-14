//
#include <bela/terminal.hpp>
#include "indicators.hpp"
#include "pkg.hpp"
#include "commands.hpp"
#include "baulk.hpp"
#include "bucket.hpp"

namespace baulk::commands {
constexpr std::wstring_view infospaces = L"              ";
void displayInfoVector(std::wstring_view name, const std::vector<std::wstring> &v) {

  if (v.empty() || name.size() + 1 > infospaces.size()) {
    return;
  }
  bela::FPrintF(stderr, L"%s:%s%s\n", name, infospaces.substr(name.size() + 1), v[0]);
  for (size_t i = 1; i < v.size(); i++) {
    bela::FPrintF(stderr, L"%s%s\n", infospaces, v[i]);
  }
}

void displayInfoVector(std::wstring_view name, const std::vector<baulk::LinkMeta> &v) {
  if (v.empty() || name.size() + 1 > infospaces.size()) {
    return;
  }
  bela::FPrintF(stderr, L"%s:%s%s\n", name, infospaces.substr(name.size() + 1), v[0].path);
  for (size_t i = 1; i < v.size(); i++) {
    bela::FPrintF(stderr, L"%s%s\n", infospaces, v[i].path);
  }
}

void displayURLs(const std::vector<std::wstring> &urls) {
  if (urls.empty()) {
    return;
  }
  size_t maxlen = 0;
  std::for_each(urls.begin(), urls.end(), [&](const std::wstring &u) { maxlen = (std::max)(maxlen, u.size()); });
  bela::terminal::terminal_size termsz;
  if ([](bela::terminal::terminal_size &termsz) -> bool {
        if (bela::terminal::IsSameTerminal(stderr)) {
          if (auto cygwinterminal = bela::terminal::IsCygwinTerminal(stderr); cygwinterminal) {
            return CygwinTerminalSize(termsz);
          }
          return bela::terminal::TerminalSize(stderr, termsz);
        }
        return false;
      }(termsz) && termsz.columns - 14 > static_cast<uint32_t>(maxlen)) {
    bela::FPrintF(stderr, L"URLs:%s%s\n", infospaces.substr(5), urls[0]);
    for (size_t i = 1; i < urls.size(); i++) {
      bela::FPrintF(stderr, L"%s%s\n", infospaces, urls[i]);
    }
    return;
  }
  bela::FPrintF(stderr, L"URLs:\n");
  for (const auto &u : urls) {
    bela::FPrintF(stderr, L"    %s\n", u);
  }
}

int PackageDisplayInfo(std::wstring_view name) {
  bela::error_code ec, oec;
  auto pkg = baulk::bucket::PackageMetaEx(name, ec);
  if (!pkg) {
    bela::FPrintF(stderr, L"\x1b[31mbaulk: %s\x1b[0m\n", ec.message);
    return 1;
  }

  bela::FPrintF(stderr,
                L"Description:  %s\nName:         \x1b[32m%s\x1b[0m\nBucket:       \x1b[34m%s\x1b[0m\nHomepage:     "
                L"%s\nLicense:      %s\nVersion:      %s\n",
                pkg->description, pkg->name, pkg->bucket, pkg->homepage, pkg->license, pkg->version);

  if (auto lpkg = baulk::bucket::PackageLocalMeta(name, oec); lpkg) {
    bela::FPrintF(stderr, L"Installed:    \x1b[36m%s\x1b[0m\n", lpkg->version);
  }
  displayInfoVector(L"Links", pkg->links);
  displayInfoVector(L"Launchers", pkg->launchers);
  displayInfoVector(L"Suggest", pkg->suggest);
  if (!pkg->notes.empty()) {
    bela::FPrintF(stderr, L"Notes:\n  %s\n", pkg->notes);
  }
  displayURLs(pkg->urls);
  return 0;
}

void usage_info() {
  bela::FPrintF(stderr, LR"(Usage: baulk info [package]...
Show package information

Example:
  baulk info wget

)");
}

int cmd_info(const argv_t &argv) {
  if (argv.empty()) {
    usage_info();
    return 1;
  }
  bela::error_code ec;
  auto locker = baulk::BaulkCloser::BaulkMakeLocker(ec);
  if (!locker) {
    bela::FPrintF(stderr, L"baulk info: \x1b[31m%s\x1b[0m\n", ec.message);
    return 1;
  }
  for (auto pkg : argv) {
    PackageDisplayInfo(pkg);
    bela::FPrintF(stderr, L"\n");
  }
  return 0;
}
} // namespace baulk::commands
