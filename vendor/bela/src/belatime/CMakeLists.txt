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
  timezone.cc)

target_link_libraries(belatime bela)

