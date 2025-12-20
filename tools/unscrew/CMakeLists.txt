#
add_executable(unscrew WIN32 unscrew.cc extractor.cc unscrew.rc unscrew.manifest)

target_link_libraries(
  unscrew
  baulk.vfs
  baulk.archive
  baulk.misc
  belashl
  belatime
  Msi
  uxtheme
  dwmapi
  comctl32
  Shell32)

install(TARGETS unscrew DESTINATION bin)
