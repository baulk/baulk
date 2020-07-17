# Author: Force Charlie Copyright (C) 2020. Force Charlie. All Rights Reserved.

add_executable(
  baulk-dock WIN32
  baulk-dock.cc
  app.cc
  appenv.cc
  baulk-dock.rc
  baulk-dock.manifest)

if(BAULK_ENABLE_LTO)
  set_property(TARGET baulk-dock PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

target_link_libraries(
  baulk-dock
  baulkenv
  belashl
  d2d1
  dwrite
  Pathcch
  shcore
  Winmm
  comctl32
  ole32.lib
  comsupp.lib)

install(TARGETS "baulk-dock" DESTINATION bin)
