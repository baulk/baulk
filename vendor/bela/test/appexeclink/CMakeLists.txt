##

add_executable(appexeclink_test
  appexeclink.cc
)

target_link_libraries(appexeclink_test
  belawin
)


add_executable(rptest
rptest.cc
)

target_link_libraries(rptest
  belawin
)
