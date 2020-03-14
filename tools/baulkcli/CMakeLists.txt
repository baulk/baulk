# baulkcli
add_executable(baulkcli baulkcli.cc baulkcli.rc baulkcli.manifest)

target_link_libraries(baulkcli belawin)

if(BAULK_ENABLE_LTO)
  set_property(TARGET baulkcli PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()
