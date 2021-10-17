# env libs

add_library(baulk.net STATIC indicators.cc client.cc speed.cc tcp.cc utils.cc)
target_link_libraries(baulk.net belawin)
