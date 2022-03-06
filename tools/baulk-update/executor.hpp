///
#include <baulk/json_utils.hpp>
#include <baulk/debug.hpp>
#include <bela/semver.hpp>

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
  bool extract_file(const std::wstring_view arfile);
  bool apply_baulk_files();
  bool replace_baulk_files();
  void cleanup();
  std::wstring download_url;
  std::wstring extract_dest;
  bela::version latest_version;
  bool forceMode{false};
  bool usePreview{false};
};
} // namespace baulk