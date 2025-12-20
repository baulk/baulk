# baulklnk
add_executable(baulk-migrate baulk-migrate.cc migrate.cc baulk-migrate.rc baulk-migrate.manifest)

target_link_libraries(baulk-migrate belawin baulk.vfs)

install(TARGETS baulk-migrate DESTINATION bin)
