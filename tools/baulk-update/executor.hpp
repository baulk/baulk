///
#include <baulk/json_utils.hpp>
#include <baulk/debug.hpp>
#include <bela/semver.hpp>
#include <filesystem>

namespace baulk {
class Executor {
public:
  Executor() = default;
  Executor(const Executor &) = delete;
  Executor &operator=(const Executor &) = delete;
  ~Executor() { cleanup(); }
  bool ParseArgv(int argc, wchar_t **argv);
  bool Execute();

private:
  bool latest_download_url();
  bool latest_download_url(const bela::version &ov);
  bool latest_download_url_fallback(const bela::version &ov);
  std::optional<std::wstring> download_file() const;
  bool extract_file(const std::filesystem::path &archive_file);
  bool apply_baulk_files();
  bool replace_baulk_files();
  static void cleanup();
  std::wstring specified;
  std::wstring download_url;
  std::filesystem::path destination;
  bela::version latest_version;
  bool forceMode{false};
};
} // namespace baulk