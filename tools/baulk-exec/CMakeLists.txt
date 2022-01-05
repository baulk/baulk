# baulkexec
add_executable(baulk-exec baulk-exec.cc executor.cc baulk-exec.rc baulk-exec.manifest)

target_link_libraries(baulk-exec baulk.vfs belatime belawin)

if(BAULK_ENABLE_LTO)
  set_property(TARGET baulk-exec PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

install(TARGETS baulk-exec DESTINATION bin)
