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

add_executable(
  vsenv_test
  vsenv.cc
  ../tools/baulk/compiler.cc
  ../tools/baulk/fs.cc)

target_link_libraries(vsenv_test belawin)

add_executable(unzip_test unzip.cc)

target_link_libraries(unzip_test baulkarchive belawin)
target_include_directories(unzip_test PRIVATE ../lib/archive)
