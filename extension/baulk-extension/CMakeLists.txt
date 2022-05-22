# shell

add_library(baulk-extension SHARED shellext.cc shellext.def shellext.rc)

if(BAULK_ENABLE_LTO)
  set_property(TARGET baulk-extension PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()


if(MSVC)
  target_compile_options(baulk-extension PRIVATE "/await")
endif()

target_link_libraries(
  baulk-extension
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