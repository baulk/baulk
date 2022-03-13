# test code
add_executable(ext_test ext.cc)
target_link_libraries(ext_test belawin)


add_executable(decodemsi_test decodemsi.cc)
target_link_libraries(decodemsi_test belawin)

add_executable(genlink_test genlinksrc.cc)
target_link_libraries(genlink_test belawin)

add_executable(parsejson_test parsejson.cc)
target_link_libraries(parsejson_test belawin)

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

target_link_libraries(vsenv_test baulk.vfs belawin)

add_executable(unzip unzip.cc)

target_link_libraries(unzip baulk.archive belawin belatime)
target_include_directories(unzip PRIVATE ../lib/archive)

add_executable(untar untar.cc)

target_link_libraries(untar baulk.archive belawin belatime)
target_include_directories(untar PRIVATE ../lib/archive)

add_executable(parsepax_test parsepax.cc)

target_link_libraries(parsepax_test belawin belatime)

add_executable(repols_test repols.cc)

target_link_libraries(repols_test belawin belatime)

add_executable(chdirtime_test chdirtime.cc)

target_link_libraries(chdirtime_test belawin belatime)

add_executable(packaged_test packaged.cc)

target_link_libraries(packaged_test belawin)

add_executable(fsmutex_test fsmutex.cc)

target_link_libraries(fsmutex_test belawin)

add_executable(indicators_test indicators_test.cc)
target_link_libraries(indicators_test baulk.net)

add_executable(pb_test pb1.cc base.manifest)
target_link_libraries(pb_test belashl)

add_executable(pb2_test pb2.cc base.manifest)
target_link_libraries(pb2_test belashl)


add_executable(titlebar_test WIN32 titlebar.cc base.manifest)
target_link_libraries(titlebar_test belashl)


add_executable(neofetch neofetch.cc base.manifest)
target_link_libraries(neofetch belatime belawin DXGI)

add_executable(pathext_test pathextension.cc base.manifest)
target_link_libraries(pathext_test belatime belawin)

add_executable(extract_test extract.cc base.manifest)
target_link_libraries(extract_test baulk.archive)

add_executable(vfsenv_test vfsenv.cc base.manifest)
target_link_libraries(vfsenv_test belawin)