##

add_executable(baulk
  7z.cc
  baulk.cc
  bucket.cc
  commands.install.cc
  commands.list.cc
  commands.search.cc
  commands.uninstall.cc
  commands.update.cc
  commands.upgrade.cc
  compiler.cc
  decompress.cc
  fs.cc
  io.cc
  launcher.cc
  msi.cc
  net.cc
  process.cc
  zip.cc
  baulk.rc
  baulk.manifest
)

target_link_libraries(baulk
  belawin
  winhttp
  Msi
  XmlLite
)

add_executable(indicators_test
  indicators_test.cc
)

target_link_libraries(indicators_test
  belawin
  winhttp
)

add_executable(windl_test
  windl_test.cc
  net.cc
)

target_link_libraries(windl_test
  belawin
  winhttp
)