
add_subdirectory(enet)
add_subdirectory(libvxl)

target_include_directories(enet PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/enet/include)
target_include_directories(libvxl PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/libvxl)
