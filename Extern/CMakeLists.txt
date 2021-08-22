
add_subdirectory(enet)
add_subdirectory(libmapvxl)

target_include_directories(enet PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/enet/include)
