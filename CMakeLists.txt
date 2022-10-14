cmake_minimum_required(VERSION 3.16)

# SpadesX
project(SpadesX)

# Find pthread
find_package(Threads REQUIRED)

find_package(PkgConfig)
pkg_check_modules(PC_enet QUIET enet)

find_path(enet_INCLUDE_DIR
  NAMES enet/enet.h
  PATHS ${PC_enet_INCLUDE_DIRS} ../../deps
  PATH_SUFFIXES enet
)

find_library(enet_LIBRARY
  NAMES enet
  PATHS ${PC_enet_LIBRARY_DIRS} ../../deps
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(enet
  FOUND_VAR enet_FOUND
  REQUIRED_VARS
    enet_LIBRARY
    enet_INCLUDE_DIR
)

set(CMAKE_EXPORT_COMPILE_COMMANDS On)

# Add third party libraries
add_subdirectory(Extern)

# Add main target
add_compile_options(-Wall -Wextra -Werror -Wpedantic -Wno-error=unused-but-set-parameter -Wno-error=pedantic -std=gnu11)
add_executable(SpadesX "")

if (enet_FOUND)
  add_library(enet::enet STATIC IMPORTED)
  set_target_properties(enet::enet PROPERTIES
    IMPORTED_LOCATION ${enet_LIBRARY}
    INTERFACE_INCLUDE_DIRECTORIES ${enet_INCLUDE_DIR}
  )
  if (WIN32)
    set_target_properties(enet::enet PROPERTIES
      INTERFACE_LINK_LIBRARIES "ws2_32;winmm"
    )
  endif (WIN32)
endif (enet_FOUND)

target_link_libraries(SpadesX
    PRIVATE
        Server
        Util
        enet::enet
        mapvxl
        m
        json-c
        readline
        Threads::Threads
)

if (UNIX)
    target_link_libraries(SpadesX PRIVATE bsd)
endif()

add_subdirectory(Source)

set_target_properties(SpadesX Util Server
  PROPERTIES
    INTERPROCEDURAL_OPTIMIZATION true
)

if (NOT EXISTS ${CMAKE_BINARY_DIR}/config.json)
configure_file(${PROJECT_SOURCE_DIR}/Resources/config.json ${CMAKE_BINARY_DIR}/config.json COPYONLY)
endif()

if (NOT EXISTS ${CMAKE_BINARY_DIR}/Border_Hallway.vxl)
configure_file(${PROJECT_SOURCE_DIR}/Resources/Border_Hallway.vxl ${CMAKE_BINARY_DIR}/Border_Hallway.vxl COPYONLY)
endif()

if (NOT EXISTS ${CMAKE_BINARY_DIR}/Border_Hallway.json)
configure_file(${PROJECT_SOURCE_DIR}/Resources/Border_Hallway.json ${CMAKE_BINARY_DIR}/Border_Hallway.json COPYONLY)
endif()
