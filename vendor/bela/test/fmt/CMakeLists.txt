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

add_executable(unicode_test
  unicodetest.cc
  ../../src/bela/unicode.cc
)

target_link_libraries(unicode_test
  bela
)

add_executable(ansi_test
  ansi.cc
)

target_link_libraries(ansi_test
  bela
)

add_executable(charconv_test
  charconv.cc
)

target_link_libraries(charconv_test
  bela
)