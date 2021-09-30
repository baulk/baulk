# env libs

add_library(baulk.env STATIC baulkenv.cc baulkvenv.cc)
target_link_libraries(baulk.env belawin)
