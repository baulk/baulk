# env libs

add_library(baulk.net STATIC best_url.cc indicators.cc net.cc tcp.cc)
target_link_libraries(baulk.net belawin)
