#

# bela win libaray

add_library(
  belatime STATIC
  clock.cc
  datetime.cc
  dos.cc
  duration.cc
  format.cc
  time.cc
  timezone.cc
  unscaledcycleclock.cc)

target_link_libraries(belatime bela)

if(BELA_ENABLE_LTO)
  set_property(TARGET belatime PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()
