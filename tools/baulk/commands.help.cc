#include <bela/subsitute.hpp>
#include <bela/terminal.hpp>
#include <bela/base.hpp>
#include <bela/io.hpp>
#include <time.hpp>
#include <jsonex.hpp>
#include "commands.hpp"
#include "fs.hpp"
#include "baulk.hpp"

namespace baulk::commands {
void Usage() {
  constexpr std::wstring_view usage = LR"(baulk - Minimal Package Manager for Windows
Usage: baulk [option] <command> [<args>]
  -h|--help        Show usage text and quit
  -v|--version     Show version number and quit
  -V|--verbose     Make the operation more talkative
  -Q|--quiet       Make the operation more quiet
  -F|--force       Turn on force mode. such as force update frozen package
  -P|--profile     Set profile path. default: $0\config\baulk.json
  -A|--user-agent  Send User-Agent <name> to server
  -k|--insecure    Allow insecure server connections when using SSL
  -T|--trace       Turn on trace mode. track baulk execution details.
  --https-proxy    Use this proxy. Equivalent to setting the environment variable 'HTTPS_PROXY'
  --force-delete   When uninstalling the package, forcefully delete the related directories


Command:
  help             Show usage text and quit
  version          Show version number and quit
  list             List all installed packages
  search           Search for available packages, or specific package details
  install          Install specific packages. upgrade if already installed. (alias: i)
  uninstall        Uninstall specific packages. (alias: r)
  update           Update ports metadata
  upgrade          Upgrade all upgradeable packages
  freeze           Freeze specific package
  unfreeze         UnFreeze specific package
  b3sum            Calculate the BLAKE3 checksum of a file
  sha256sum        Calculate the SHA256 checksum of a file
  cleancache       Cleanup download cache
  bucket           Add, delete or list buckets
  untar            Extract files in a tar archive. support: tar.xz tar.bz2 tar.gz tar.zstd (experimental)
  unzip            Extract compressed files in a ZIP archive (experimental)

Alias:
  i  install
  r  uninstall
  u  update and upgrade

)";
  bela::error_code ec;
  auto exeparent = bela::ExecutableParent(ec);
  if (!exeparent) {
    auto msg = bela::Substitute(usage, L"$Prefix");
    bela::terminal::WriteAuto(stderr, msg);
    return;
  }
  bela::PathStripName(*exeparent);
  auto msg = bela::Substitute(usage, *exeparent);
  bela::terminal::WriteAuto(stderr, msg);
}

int cmd_help(const baulk::commands::argv_t &argv) {
  if (argv.empty()) {
    Usage();
    return 0;
  }
  return 0;
}
} // namespace baulk::commands