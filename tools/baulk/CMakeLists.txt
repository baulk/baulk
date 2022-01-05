#

add_executable(
  baulk
  7z.cc
  baulk.cc
  bucket.cc
  commands.b3sum.cc
  commands.brand.cc
  commands.bucket.cc
  commands.cleancache.cc
  commands.extract.cc
  commands.freeze.cc
  commands.help.cc
  commands.info.cc
  commands.install.cc
  commands.list.cc
  commands.search.cc
  commands.sha256sum.cc
  commands.uninstall.cc
  commands.untar.cc
  commands.unzip.cc
  commands.update.cc
  commands.upgrade.cc
  commands.version.cc
  compiler.cc
  context.cc
  decompress.cc
  launcher.cc
  msi.cc
  pkg.cc
  tar.cc
  baulk.rc
  baulk.manifest)

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
  Msi)

if(BAULK_ENABLE_LTO)
  set_property(TARGET baulk PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

target_include_directories(baulk PRIVATE ../../lib/archive)

install(TARGETS baulk DESTINATION bin)
