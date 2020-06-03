# baulkexec
add_executable(baulkexec baulkexec.cc baulkexec.rc baulkexec.manifest)

target_link_libraries(baulkexec baulkenv belawin)

if(BAULK_ENABLE_LTO)
  set_property(TARGET baulkexec PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

install(TARGETS baulkexec DESTINATION bin)
