cmake_minimum_required(VERSION 3.16)

# SpadesX
project(SpadesX)

if(WIN32)
    if (NOT "${VCPKG_INSTALLED_DIR}" STREQUAL "")
        if("${CMAKE_C_COMPILER_ID}" STREQUAL "MSVC" OR "CLANG")
            find_package(unofficial-enet CONFIG REQUIRED)
            find_package(json-c CONFIG REQUIRED)
            find_package(PThreads4W REQUIRED)
            find_package(unofficial-readline-win32 CONFIG REQUIRED)

            add_subdirectory(Extern)

            add_compile_options(/W4 /std:c11) #/WX
            add_executable(SpadesX "")

            set_target_properties(unofficial::enet::enet PROPERTIES
                INTERFACE_LINK_LIBRARIES "ws2_32;winmm"
            )

            target_link_libraries(SpadesX
                PRIVATE
                    unofficial::readline-win32::readline
                    PThreads4W::PThreads4W
                    json-c::json-c
                    unofficial::enet::enet
                    util
                    mapvxl
            )

            add_subdirectory(Source)
        else()
            message(FATAL_ERROR "Sorry, but compiling without MSVC or Clang on Windows is not yet supported.")
        endif()
    else()
        message(FATAL_ERROR "Sorry, but compiling using CMake on Windows without Visual Studio and VCPKG is not yet supported.")
    endif()
else()
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
endif()
