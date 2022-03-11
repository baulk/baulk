//
#ifndef BAULK_MIGRATE_HPP
#define BAULK_MIGRATE_HPP
#include <vector>
#include <bela/base.hpp>
#include <baulk/debug.hpp>

namespace baulk {
//
class Migrator {
public:
  Migrator() = default;
  Migrator(const Migrator &) = delete;
  Migrator &operator=(const Migrator &) = delete;
  int Execute();
  int PostMigrate();

private:
  std::vector<std::wstring> pkgs;
  bool make_link_once(std::wstring_view pkgName);
};
} // namespace baulk

#endif