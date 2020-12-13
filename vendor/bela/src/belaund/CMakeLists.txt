# bela win libaray

add_library(
  belaund STATIC
  llvm/Demangle.cpp
  llvm/ItaniumDemangle.cpp
  llvm/MicrosoftDemangle.cpp
  llvm/MicrosoftDemangleNodes.cpp)

if(BELA_ENABLE_LTO)
  set_property(TARGET belaund PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()
