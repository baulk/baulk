#

set(PPMD_LIBRARY_SOURCES
    CpuArch.c
    Ppmd7.c
    Ppmd8.c
    Ppmd7aDec.c
    Ppmd7Dec.c
    Ppmd7Enc.c
    Ppmd8Dec.c
    Ppmd8Enc.c)

add_library(ppmd STATIC ${PPMD_LIBRARY_SOURCES})

target_include_directories(ppmd PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})