#

#
add_executable(ssh-askpass-baulk main.cc ssh-askpass-baulk.rc ssh-askpass-baulk.manifest)

target_link_libraries(ssh-askpass-baulk belashl Shell32)

if(BAULK_ENABLE_LTO)
  set_property(TARGET ssh-askpass-baulk PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

install(TARGETS ssh-askpass-baulk DESTINATION "bin")
