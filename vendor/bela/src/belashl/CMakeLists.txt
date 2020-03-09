# bela win libaray

add_library(belashl STATIC
 picker.cc
)

target_link_libraries(belashl
  belawin
  Comctl32
  Ole32
)
