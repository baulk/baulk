#

add_executable(baulk-update baulk-update.cc zip.cc baulk-update.rc baulk-update.manifest)

target_link_libraries(
  baulk-update
  baulk.archive
  baulk.net
  baulk.vfs
  baulk.misc
  belawin
  belatime
  winhttp
  ws2_32)

if(BAULK_ENABLE_LTO)
  set_property(TARGET baulk-update PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

target_include_directories(baulk-update PRIVATE ../../lib/archive)
install(TARGETS baulk-update DESTINATION bin)
