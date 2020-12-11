# bela base libaray

add_library(
  bela STATIC
  errno.cc
  ascii.cc
  ucwidth.cc
  codecvt.cc
  escaping.cc
  fmt.cc
  fnmatch.cc
  match.cc
  memutil.cc
  numbers.cc
  winansi.cc
  str_split.cc
  str_replace.cc
  strcat.cc
  strcat_narrow.cc
  subsitute.cc
  terminal.cc)

if(BELA_ENABLE_LTO)
  set_property(TARGET bela PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()
