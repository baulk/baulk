##
add_executable(pecoff_test
  pecoff.cc
)

target_link_libraries(pecoff_test
  belawin
  belaund
)

##
add_executable(version_test
  version.cc
)

target_link_libraries(version_test
  belawin
)


add_executable(pick_test
  pick.cc
  ../../res/win10.manifest
)

target_link_libraries(pick_test
  belashl
)