# shell

add_library(unscrew-extension SHARED shellext.cc shellext.def shellext.rc)

target_link_libraries(
  unscrew-extension
  runtimeobject.lib
  winspool.lib
  comdlg32.lib
  advapi32.lib
  shell32.lib
  ole32.lib
  oleaut32.lib
  uuid.lib
  odbc32.lib
  odbccp32.lib
  shlwapi.lib
  belawin)