#
add_executable(
  baulk-terminal WIN32
  baulk-terminal.cc
  env.cc
  trace.cc
  baulk-terminal.rc
  baulk-terminal.manifest)

target_link_libraries(baulk-terminal belashl belatime Shell32)

if(BAULK_ENABLE_LTO)
  set_property(TARGET baulk-terminal PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

install(TARGETS baulk-terminal DESTINATION ".")
