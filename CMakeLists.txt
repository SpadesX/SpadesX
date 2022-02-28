cmake_minimum_required(VERSION 3.16)

#
# SpadesX
#

project(SpadesX)

#
# Add third party libraries
#

add_subdirectory(Extern)

#
# Add main target
#
add_compile_options(-Wall -Wextra -Wpedantic -Werror -Wno-error=unused-but-set-parameter -O2 -std=gnu11)
add_executable(SpadesX "")

target_link_libraries(SpadesX
    PRIVATE
        enet
        util
        mapvxl
        m
        json-c
        pthread
        bsd
)

add_subdirectory(Source)
