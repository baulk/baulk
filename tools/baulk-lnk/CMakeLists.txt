# baulklnk
add_executable(baulk-lnk baulk-lnk.cc baulk-lnk.rc baulk-lnk.manifest)

target_link_libraries(baulk-lnk belawin)

add_executable(baulk-winlnk WIN32 baulk-winlnk.cc baulk-winlnk.rc baulk-lnk.manifest)

target_link_libraries(baulk-winlnk belawin belashl)

install(TARGETS baulk-lnk DESTINATION bin)
install(TARGETS baulk-winlnk DESTINATION bin)
