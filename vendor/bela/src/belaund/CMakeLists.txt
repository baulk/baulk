# bela win libaray

add_library(
  belaund STATIC
  llvm/lib/Demangle/Demangle.cpp
  llvm/lib/Demangle/DLangDemangle.cpp
  llvm/lib/Demangle/ItaniumDemangle.cpp
  llvm/lib/Demangle/MicrosoftDemangle.cpp
  llvm/lib/Demangle/MicrosoftDemangleNodes.cpp
  llvm/lib/Demangle/RustDemangle.cpp)

if(BELA_ENABLE_LTO)
  set_property(TARGET belaund PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

target_include_directories(belaund PRIVATE llvm/include)
target_compile_options(belaund PRIVATE -wd4996 -wd4244 -wd4267)