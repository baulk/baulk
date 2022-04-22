#include <bela/env.hpp>
#include <bela/path.hpp>
#include <bela/io.hpp>
#include <baulk/fs.hpp>
#include <baulk/vs.hpp>
#include "compiler.hpp"
#include "baulk.hpp"

namespace baulk::compiler {

bool Executor::Initialize(bela::error_code &init_ec) {
  simulator.InitializeCleanupEnv();
  if (auto vs = baulk::env::InitializeVisualStudioEnv(simulator, baulk::env::HostArch, false, init_ec); vs) {
    DbgPrint(L"initialize vs env success: %v", *vs);
    initialized = true;
    return true;
  }
  DbgPrint(L"initialize vs env error: %v", init_ec);
  return false;
}
} // namespace baulk::compiler