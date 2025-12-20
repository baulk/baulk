# bela win libaray

add_library(
  belaund STATIC
  llvm/lib/Demangle/Demangle.cpp
  llvm/lib/Demangle/DLangDemangle.cpp
  llvm/lib/Demangle/ItaniumDemangle.cpp
  llvm/lib/Demangle/MicrosoftDemangle.cpp
  llvm/lib/Demangle/MicrosoftDemangleNodes.cpp
  llvm/lib/Demangle/RustDemangle.cpp)

target_include_directories(belaund PRIVATE llvm/include)
target_compile_options(belaund PRIVATE -wd4996 -wd4244 -wd4267)