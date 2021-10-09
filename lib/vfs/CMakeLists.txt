# env libs

add_library(baulk.vfs STATIC export.cc manifest.cc table.cc vfs.cc)
target_link_libraries(baulk.vfs belawin)
