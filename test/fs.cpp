#include <iostream>
#include <filesystem>
namespace fs = std::filesystem;
 
int main()
{
    std::cout << "Current root name is: " << fs::current_path().root_name() << '\n';
}