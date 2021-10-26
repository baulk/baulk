#
add_executable(unscrew WIN32 unscrew.cc progressbar.cc unscrew.rc unscrew.manifest)

target_link_libraries(unscrew belashl belatime Shell32)

if(BAULK_ENABLE_LTO)
  set_property(TARGET unscrew PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

install(TARGETS unscrew DESTINATION "bin")
