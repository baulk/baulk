# baulklnk
add_executable(baulk-lnk baulk-lnk.cc baulk-lnk.rc baulk-lnk.manifest)

target_link_libraries(baulk-lnk belawin)

add_executable(baulk-winlnk WIN32 baulk-winlnk.cc baulk-winlnk.rc baulk-lnk.manifest)

target_link_libraries(baulk-winlnk belawin belashl)

if(BAULK_ENABLE_LTO)
  set_property(TARGET baulk-lnk PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
  set_property(TARGET baulk-winlnk PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

install(TARGETS baulk-lnk DESTINATION bin)
install(TARGETS baulk-winlnk DESTINATION bin)
