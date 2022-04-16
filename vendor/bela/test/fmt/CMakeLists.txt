##

add_executable(fmt_test
  main.cc
)

target_link_libraries(fmt_test
  bela
)


add_executable(strjoin_test
  strjoin.cc
)

target_link_libraries(strjoin_test
  bela
)

add_executable(charconv_test
  charconv.cc
)

target_link_libraries(charconv_test
  bela
)