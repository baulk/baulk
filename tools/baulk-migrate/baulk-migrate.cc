/// migrate legacy mode to new Portable mode
#include "migrate.hpp"

int wmain(int argc, wchar_t **argv) {
  baulk::Migrator migrator;
  if (migrator.Execute() != 0) {
    return 1;
  }
  return migrator.PostMigrate();
}