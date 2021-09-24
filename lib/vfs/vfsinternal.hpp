///
#include <atomic>
#include <baulk/vfs.hpp>

namespace baulk::vfs {
class FsProvider {
public:
  FsProvider(const FsProvider &) = delete;
  FsProvider &operator=(const FsProvider &) = delete;
  static FsProvider &Instance();
  bool Initialize(bela::error_code &ec);
  const PathMappingTable &PathTable() { return table; }

private:
  FsProvider() {}
  PathMappingTable table;
  std::atomic_bool initialized{false};
};
} // namespace baulk::vfs