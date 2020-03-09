##

add_executable(toml_test
  toml.cc
)

target_link_libraries(toml_test
  bela
  belawin
)


add_executable(cpptoml_test
  cpptoml.cc
)

target_link_libraries(cpptoml_test
  bela
  belawin
)
