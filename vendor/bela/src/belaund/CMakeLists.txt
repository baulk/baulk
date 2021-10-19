# bela win libaray

add_library(
  belaund STATIC
  llvm/Demangle.cpp
  llvm/ItaniumDemangle.cpp
  llvm/MicrosoftDemangle.cpp
  llvm/MicrosoftDemangleNodes.cpp
  llvm/RustDemangle.cpp)

if(BELA_ENABLE_LTO)
  set_property(TARGET belaund PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

target_compile_options(belaund PRIVATE -wd4996 -wd4244 -wd4267)