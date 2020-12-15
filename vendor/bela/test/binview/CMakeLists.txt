# base
add_executable(elfview_test
  elfview.cc
)

target_link_libraries(elfview_test
  hazel
  belaund
)

# base
add_executable(machoview_test
  machoview.cc
)

target_link_libraries(machoview_test
  hazel
  belaund
)