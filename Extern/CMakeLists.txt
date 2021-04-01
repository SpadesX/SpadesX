
add_subdirectory(enet)
add_subdirectory(zlib)

target_include_directories(enet PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/enet/include)
target_include_directories(zlib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/zlib)
