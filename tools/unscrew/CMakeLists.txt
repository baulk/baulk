#
add_executable(unscrew WIN32 unscrew.cc window.cc utils.cc unscrew.rc unscrew.manifest)

target_link_libraries(unscrew belashl belatime uxtheme dwmapi comctl32 Shell32)

if(BAULK_ENABLE_LTO)
  set_property(TARGET unscrew PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

install(TARGETS unscrew DESTINATION "bin")
