#
add_executable(baulkterminal WIN32 baulkterminal.cc baulkterminal.rc baulkterminal.manifest)

target_link_libraries(baulkterminal belashl)

if(BAULK_ENABLE_LTO)
  set_property(TARGET baulkterminal PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

install(TARGETS baulkterminal DESTINATION ".")