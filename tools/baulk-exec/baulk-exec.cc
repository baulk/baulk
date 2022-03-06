///
#include <bela/terminal.hpp>
#include <bela/parseargv.hpp>
#include <bela/pe.hpp>
#include <baulk/vfs.hpp>
#include <baulk/vs.hpp>
#include <baulk/venv.hpp>
#include <version.hpp>
#include "baulk-exec.hpp"

namespace baulk {
bool IsDebugMode = false;

// Usage
void Usage() {
  constexpr std::wstring_view usage = LR"(baulk-exec - Baulk extend executor
Usage: baulk-exec [option] <command> [<args>] ...
  -h|--help            Show usage text and quit
  -v|--version         Show version number and quit
  -V|--verbose         Make the operation more talkative
  -C|--cleanup         Create clean environment variables to avoid interference
  -W|--cwd             Set the command startup directory
  -A|--arch            Select a specific arch, use native architecture by default
  -E|--venv            Choose to load a specific package virtual environment
  --vs                 Load Visual Studio related environment variables
  --vs-preview         Load Visual Studio (Preview) related environment variables
  --time               Summarize command system resource usage

Example:
  baulk-exec -V --vs TUNNEL_DEBUG=1 pwsh

Built-in alias:
  winsh          A fake shell. It may be pwsh or powershell and cmd.
  pwsh           PowerShell Core
  pwsh-preview   PowerShell Core Preview

)";
  bela::terminal::WriteAuto(stderr, usage);
}

void Version() {
  bela::FPrintF(stdout, LR"(baulk-exec %s
Release:    %d.%d.%d.%d
Commit:     %s
Build Time: %s
)",
                BAULK_VERSION,                                                                      // version short
                BAULK_VERSION_MAJOR, BAULK_VERSION_MINOR, BAULK_VERSION_PATCH, BAULK_VERSION_BUILD, // version full
                BAULK_REVISION,                                                                     // version commit
                BAULK_BUILD_TIME                                                                    // build time
  );
}

bool InitializeGitEnv(bool isCleanupEnv, bela::env::Simulator &sm, bela::error_code &ec) {
  std::wstring git;
  if (bela::env::LookPath(L"git.exe", git, true)) {
    if (!isCleanupEnv) {
      return true;
    }
    bela::PathStripName(git);
    sm.PathAppend(git);
    return true;
  }
  auto installPath = baulk::registry::GitForWindowsInstallPath(ec);
  if (!installPath) {
    return false;
  }
  git = bela::StringCat(*installPath, L"\\cmd\\git.exe");
  if (!bela::PathExists(git)) {
    return false;
  }
  sm.PathAppend(bela::StringCat(*installPath, L"\\cmd"));
  return true;
}

// ParseArgv todo
bool Executor::ParseArgv(int argc, wchar_t **cargv) {
  bela::ParseArgv pa(argc, cargv, true);
  pa.Add(L"help", bela::no_argument, L'h')
      .Add(L"version", bela::no_argument, L'v')
      .Add(L"verbose", bela::no_argument, L'V')
      .Add(L"cleanup", bela::no_argument, L'C') // cleanup environment
      .Add(L"arch", bela::required_argument, L'A')
      .Add(L"cwd", bela::required_argument, L'W')
      .Add(L"venv", bela::required_argument, L'E')
      .Add(L"vs", bela::no_argument, 1000) // load visual studio environment
      .Add(L"vs-preview", bela::no_argument, 1001)
      .Add(L"clang", bela::no_argument, 1002)
      .Add(L"time", bela::no_argument, 1004);
  bool initializeVSEnv = false;
  bool initializeVSPreviewEnv = false;
  std::wstring arch(baulk::env::HostArch);
  std::vector<std::wstring> packageEnvs;
  bela::error_code ec;
  auto ret = pa.Execute(
      [&](int val, const wchar_t *oa, const wchar_t *) {
        switch (val) {
        case 'h':
          Usage();
          exit(0);
        case 'v':
          Version();
          exit(0);
        case 'C':
          cleanup = true;
          break;
        case 'V':
          IsDebugMode = true;
          break;
        case 'W':
          cwd = oa;
          break;
        case 'A': {
          auto larch = bela::AsciiStrToLower(oa);
          if (larch == L"x64" || larch == L"arm64" || larch == L"x86" || larch == L"arm") {
            arch = larch;
            break;
          }
          bela::FPrintF(stderr, L"baulk-exec: unsupport arch '%s'\n", oa);
        } break;
        case 'E':
          packageEnvs.emplace_back(oa);
          break;
        case 1000:
          if (!initializeVSPreviewEnv) {
            initializeVSEnv = true;
          }
          break;
        case 1001:
          if (!initializeVSEnv) {
            initializeVSPreviewEnv = true;
          }
          break;
          break;
        case 1004:
          summarizeTime = true;
          break;
        default:
          break;
        }
        return true;
      },
      ec);
  if (!ret) {
    bela::FPrintF(stderr, L"baulk-exec: parse argv \x1b[31m%s\x1b[0m\n", ec);
    return false;
  }
  const auto &Argv = pa.UnresolvedArgs();
  if (pa.UnresolvedArgs().empty()) {
    bela::FPrintF(stderr, L"baulk-exec: no command input\n");
    Usage();
    exit(1);
  }
  if (cleanup) {
    DbgPrint(L"baulk-exec: use cleaned env");
    simulator.InitializeCleanupEnv();
  } else {
    simulator.InitializeEnv();
  }

  if (!baulk::vfs::InitializeFastPathFs(ec)) {
    bela::FPrintF(stderr, L"baulk-exec InitializeFastPathFs error %s\n", ec);
    return false;
  }

  for (size_t i = 0; i < Argv.size(); i++) {
    const auto arg = Argv[i];
    // support KEY=VALUE env setter
    auto pos = arg.find(L'=');
    if (pos == std::wstring::npos) {
      // FIND ARGS
      for (size_t j = i; j < Argv.size(); j++) {
        argv.emplace_back(Argv[j]);
      }
      // BREAK ARGS PARSE
      break;
    }
    simulator.SetEnv(arg.substr(0, pos), arg.substr(pos + 1));
  }
  if (!InitializeGitEnv(cleanup, simulator, ec)) {
    bela::FPrintF(stderr, L"initialize git env failed: %s\n", ec);
  }

  if (initializeVSEnv) {
    if (!baulk::env::InitializeVisualStudioEnv(simulator, arch, false, ec)) {
      bela::FPrintF(stderr, L"Initialize visual studio env failed %s\n", ec);
    } else {
      DbgPrint(L"Initialize visual studio env done");
    }
  }
  if (initializeVSPreviewEnv) {
    if (!baulk::env::InitializeVisualStudioEnv(simulator, arch, true, ec)) {
      bela::FPrintF(stderr, L"Initialize visual studio (Preview) env failed %s\n", ec);
    } else {
      DbgPrint(L"Initialize visual studio (Preview) env done");
    }
  }

  std::vector<std::wstring> paths = {baulk::vfs::AppLocationPath(), std::wstring(baulk::vfs::AppLinks())};
  if (IsDebugMode) {
    for (const auto &p : paths) {
      DbgPrint(L"Path insert %s", p);
    }
  }
  simulator.PathPushFront(std::move(paths));
  baulk::env::Constructor ctor(baulk::IsDebugMode);
  if (!ctor.InitializeEnvs(packageEnvs, simulator, ec)) {
    bela::FPrintF(stderr, L"baulk-exec constructor venvs error %s\n", ec);
    return false;
  }
  return true;
}
} // namespace baulk

int wmain(int argc, wchar_t **argv) {
  baulk::Executor executor;
  if (!executor.ParseArgv(argc, argv)) {
    return 1;
  }
  return executor.Exec();
}
