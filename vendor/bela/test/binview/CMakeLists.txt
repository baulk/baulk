# base
add_executable(elfview_test
  elfview.cc
)

target_link_libraries(elfview_test
  hazel
)