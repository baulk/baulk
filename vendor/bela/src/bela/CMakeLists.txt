# bela base libaray

add_library(
  bela STATIC
  errno.cc
  ascii.cc
  city.cc
  codecvt.cc
  escaping.cc
  int128.cc
  match.cc
  numbers.cc
  str_split.cc
  str_split_narrow.cc
  str_replace.cc
  str_replace_narrow.cc
  str_cat.cc
  subsitute.cc
  terminal.cc
  __charconv/charconv.cc
  __charconv/charconv_float.cc
  __fnmatch/fnmatch.cc
  __format/fmt.cc)

if(BELA_ENABLE_LTO)
  set_property(TARGET bela PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()
