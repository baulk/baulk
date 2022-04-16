# wind
add_executable(wind wind.cc wind.rc wind.manifest)

target_link_libraries(
  wind
  baulk.net
  baulk.misc
  belatime
  belahash
  belawin
  winhttp
  ws2_32
  Msi)

install(TARGETS wind DESTINATION bin)
