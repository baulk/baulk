#
file(GLOB BAULK_SOURCES *.cc)
add_executable(baulk ${BAULK_SOURCES} baulk.rc baulk.manifest)

target_link_libraries(
  baulk
  baulk.archive
  baulk.misc
  baulk.net
  baulk.vfs
  belahash
  belawin
  belatime
  winhttp
  ws2_32
  DXGI
  wbemuuid
  Propsys
  Msi)

target_include_directories(baulk PRIVATE ../../lib/archive)

install(TARGETS baulk DESTINATION bin)
