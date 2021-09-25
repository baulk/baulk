///
#include <bela/simulator.hpp>
#include <bela/path.hpp>
#include <bela/io.hpp>

namespace baulk::env {
#ifdef _M_X64
// Always build x64 binary
[[maybe_unused]] constexpr std::wstring_view DefaultArch = L"x64"; // Hostx64 x64
#else
[[maybe_unused]] constexpr std::wstring_view DefaultArch = L"x86"; // Hostx86 x86
#endif
namespace env_internal {
struct vs_instance {
  std::wstring path;
  std::wstring version;
  std::wstring instance_id;
  std::wstring product_id;
  bool is_launchable{true};
  bool is_prerelease{false};
};
} // namespace env_internal
inline bool InitializeVisualStudioEnv(bela::env::Simulator &simulator, const std::wstring_view arch,
                                      const bool usePreview, bela::error_code &ec) {
  //
  return true;
}
} // namespace baulk::env