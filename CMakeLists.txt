cmake_minimum_required(VERSION 3.16)

#
# SpadesX
#

project(SpadesX)

#
# Find pthread
#
find_package(Threads REQUIRED)

#
# Add third party libraries
#

add_subdirectory(Extern)

#
# Add main target
#
add_compile_options(-Wall -Wextra -Wpedantic -Werror -Wno-error=unused-but-set-parameter -std=gnu11)
add_executable(SpadesX "")

target_link_libraries(SpadesX
    PRIVATE
        enet
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
