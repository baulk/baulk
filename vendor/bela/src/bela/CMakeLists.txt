# bela base libaray

add_library(
  bela STATIC
  errno.cc
  ascii.cc
  city.cc
  ucwidth.cc
  codecvt.cc
  escaping.cc
  fmt.cc
  fnmatch.cc
  int128.cc
  match.cc
  memutil.cc
  numbers.cc
  winansi.cc
  str_split.cc
  str_split_narrow.cc
  str_replace.cc
  str_replace_narrow.cc
  str_cat.cc
  str_cat_narrow.cc
  subsitute.cc
  subsitute_narrow.cc
  terminal.cc)

if(BELA_ENABLE_LTO)
  set_property(TARGET bela PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()
