# env libs

add_library(baulk.net STATIC client.cc speed.cc tcp.cc utils.cc)
target_link_libraries(baulk.net baulk.mem belawin)
