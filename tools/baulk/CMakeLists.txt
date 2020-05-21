#

add_executable(
  baulk
  7z.cc
  baulk.cc
  baulkenv.cc
  bucket.cc
  commands.exec.cc
  commands.freeze.cc
  commands.install.cc
  commands.list.cc
  commands.search.cc
  commands.uninstall.cc
  commands.update.cc
  commands.upgrade.cc
  commands.b3sum.cc
  commands.sha256sum.cc
  compiler.cc
  decompress.cc
  fs.cc
  hash.cc
  indicators.cc
  launcher.cc
  msi.cc
  net.cc
  pkg.cc
  tar.cc
  tcp.cc
  zip.cc
  baulk.rc
  baulk.manifest)

target_link_libraries(
  baulk
  baulkarchive
  blake3
  belawin
  winhttp
  ws2_32
  Msi
  XmlLite)

if(BAULK_ENABLE_LTO)
  set_property(TARGET baulk PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

target_include_directories(baulk PRIVATE ../../lib/archive)

install(TARGETS baulk DESTINATION bin)

if(BUILD_TEST)
  add_executable(indicators_test indicators_test.cc indicators.cc)

  target_link_libraries(indicators_test belawin winhttp)

  add_executable(windl_test windl_test.cc indicators.cc net.cc tcp.cc)

  target_link_libraries(windl_test belawin winhttp ws2_32)
endif(BUILD_TEST)
