#

add_executable(escape_test
  escape.cc
)

target_link_libraries(escape_test
  bela
)

add_executable(title_test
  settitle.cc
)

target_link_libraries(title_test
  bela
)