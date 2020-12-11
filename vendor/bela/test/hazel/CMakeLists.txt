add_executable(zipview
  zipview.cc
)

target_link_libraries(zipview
  belatime
  belawin
  hazel
)

# add_executable(shebang-gen
#   shebang-gen.cc
# )

# target_link_libraries(shebang-gen
#   belawin
# )