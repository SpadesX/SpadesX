find_package(Git REQUIRED)

if(NOT EXISTS libmapvxl/CMakeLists.txt OR NOT EXISTS spadesx_enet/CMakeLists.txt)
  execute_process(COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive -- ${dir}
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    COMMAND_ERROR_IS_FATAL ANY)
endif()

add_subdirectory(libmapvxl)
add_subdirectory(spadesx_enet)
