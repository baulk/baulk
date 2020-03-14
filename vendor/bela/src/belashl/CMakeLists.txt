# bela win libaray

add_library(belashl STATIC
 picker.cc
)

target_link_libraries(belashl
  belawin
  Comctl32
  Ole32
)

if(BELA_ENABLE_LTO)
  set_property(TARGET belashl PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()
