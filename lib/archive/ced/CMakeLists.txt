#
set(CED_LIBRARY_SOURCES
    compact_enc_det/compact_enc_det.cc
    compact_enc_det/compact_enc_det_hint_code.cc
    util/encodings/encodings.cc
    util/languages/languages.cc
    )

add_library(ced STATIC ${CED_LIBRARY_SOURCES})

target_include_directories(ced PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})