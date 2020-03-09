##

add_executable(pe_test
  pe.cc
)

target_link_libraries(pe_test
  belawin
)


add_executable(pick_test
  pick.cc
  ../../res/win10.manifest
)

target_link_libraries(pick_test
  belashl
)