# misc libs

add_library(baulk.misc STATIC fs.cc hash.cc)
target_link_libraries(baulk.misc belawin belahash)
