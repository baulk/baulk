//
#include <baulkrev.hpp>
#include <bela/terminal.hpp>
#include <filesystem>
#include "baulk.hpp"
#include "commands.hpp"

namespace baulk::commands {
void Version() {
  bela::FPrintF(stdout, LR"(Baulk - Minimal Package Manager for Windows  [%s]
InstallTarget: %s
Release:       %s
Commit:        %s
Build Time:    %s
)",
                BAULK_VERSION, baulk::InstallModeName(baulk::FindInstallMode()), BAULK_REFNAME, BAULK_REVISION,
                BAULK_BUILD_TIME);
}

int cmd_version(const baulk::commands::argv_t &) {
  Version();
  return 0;
}

} // namespace baulk::commands