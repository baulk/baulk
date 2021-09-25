# test code
add_executable(ext_test ext.cc)
target_link_libraries(ext_test belawin)


add_executable(decodemsi_test decodemsi.cc)
target_link_libraries(decodemsi_test belawin)

add_executable(genlink_test genlinksrc.cc)
target_link_libraries(genlink_test belawin)

add_executable(parsejson_test parsejson.cc)
target_link_libraries(parsejson_test belawin)

add_executable(regsdk_test regsdk.cc)
target_link_libraries(regsdk_test belawin)

add_executable(udir_test udir.cc)
target_link_libraries(udir_test belawin)

add_executable(version_test xversion.cc)
target_link_libraries(version_test belawin)

add_executable(linkmeta_test linkmeta.cc)
target_link_libraries(linkmeta_test belawin)

add_executable(find7z_test find7z.cc)
target_link_libraries(find7z_test belawin)

add_executable(out_test out.cc)
target_link_libraries(out_test belawin)

add_executable(
  vsenv_test
  vsenv.cc)

target_link_libraries(vsenv_test belawin)

add_executable(unzip unzip.cc)

target_link_libraries(unzip baulkarchive belawin belatime)
target_include_directories(unzip PRIVATE ../lib/archive)

add_executable(untar untar.cc)

target_link_libraries(untar baulkarchive belawin belatime)
target_include_directories(untar PRIVATE ../lib/archive)

add_executable(parsepax_test parsepax.cc)

target_link_libraries(parsepax_test belawin belatime)

add_executable(repols_test repols.cc)

target_link_libraries(repols_test belawin belatime)

add_executable(chdirtime_test chdirtime.cc)

target_link_libraries(chdirtime_test belawin belatime)