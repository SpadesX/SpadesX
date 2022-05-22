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
        enet::enet
        util
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
