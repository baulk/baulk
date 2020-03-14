# baulkcli
add_executable(baulkcli
  baulkcli.cc
  baulkcli.rc
  baulkcli.manifest
)

target_link_libraries(baulkcli
  belawin
)