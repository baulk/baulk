##

add_executable(exepath_test
  exepath.cc
)

target_link_libraries(exepath_test
  belawin
)

add_executable(pathcat_test
  pathcat.cc
)

target_link_libraries(pathcat_test
  belawin
)

add_executable(which
  which.cc
)

add_executable(env2
  env2.cc
)

add_executable(envps
  envps.cc
)


target_link_libraries(which
  belawin
)

add_executable(expandenv
  expandenv.cc
)

target_link_libraries(expandenv
  belawin
)

target_link_libraries(env2
  belawin
)

target_link_libraries(envps
  belawin
)


add_executable(writefile_test
  writefile.cc
)

target_link_libraries(writefile_test
  belawin
)

add_executable(process_test
  process.cc
)

target_link_libraries(process_test
  belawin
)
