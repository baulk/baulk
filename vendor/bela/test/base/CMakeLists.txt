# base
add_executable(base_test
  base.cc
)

target_link_libraries(base_test
  bela
)

# base
add_executable(fnmatch_test
  fnmatch.cc
)

target_link_libraries(fnmatch_test
  bela
)

# delfile
add_executable(delfile_test
  delfile.cc
)

target_link_libraries(delfile_test
  bela
  belawin
)

# base
add_executable(strsplit_test
  strsplit.cc
)

target_link_libraries(strsplit_test
  bela
)



# base
add_executable(startswith_test
  startswith.cc
)

target_link_libraries(startswith_test
  bela
)