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

add_executable(unicode_width_test
  unicode-width.cc
)

target_link_libraries(unicode_width_test
  bela
)