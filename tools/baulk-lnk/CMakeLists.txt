# baulklnk
add_executable(baulk-lnk baulk-lnk.cc baulk-lnk.rc baulk-lnk.manifest)

target_link_libraries(baulk-lnk belawin)

if(BAULK_ENABLE_LTO)
  set_property(TARGET baulk-lnk PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

install(TARGETS baulk-lnk DESTINATION bin)
