#include <iostream>
#include <filesystem>
namespace fs = std::filesystem;

inline std::string_view StripExtension(std::string_view filename) {
  auto pos = filename.rfind('.');
  if (pos == std::string_view::npos) {
    return filename;
  }
  return filename.substr(0, pos);
}

int main() {
  std::cout << "cl.exe " << StripExtension("cl.exe") << std::endl;
  std::cout << "Current root name is: " << fs::current_path().root_name()
            << std::endl;
}