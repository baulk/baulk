# baulklnk
add_executable(baulk-migrate baulk-migrate.cc baulk-migrate.rc baulk-migrate.manifest)

target_link_libraries(baulk-migrate belawin baulk.vfs)

if(BAULK_ENABLE_LTO)
  set_property(TARGET baulk-migrate PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

install(TARGETS baulk-migrate DESTINATION bin)
