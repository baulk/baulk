//
#include <version.hpp>
#include <bela/terminal.hpp>
#include <filesystem>
#include <baulk/vfs.hpp>
#include "baulk.hpp"
#include "commands.hpp"

namespace baulk::commands {
void Version() {
  bela::FPrintF(stdout, LR"(Baulk - Minimal Package Manager for Windows [%s]
Mode:       %s
Version:    %d.%d.%d.%d
Branch:     %s
Commit:     %s
BuildTime:  %s
)",
                BAULK_VERSION,                                                                      // version short
                vfs::AppMode(),                                                                     // AppMode
                BAULK_VERSION_MAJOR, BAULK_VERSION_MINOR, BAULK_VERSION_PATCH, BAULK_VERSION_BUILD, // version full
                BAULK_REFNAME,   // version for refname
                BAULK_REVISION,  // version commit
                BAULK_BUILD_TIME // build time
  );
}

int cmd_version(const baulk::commands::argv_t & /*unused*/) {
  Version();
  return 0;
}

} // namespace baulk::commands