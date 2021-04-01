cmake_minimum_required(VERSION 3.18)

#
# BetterFasterSpades
#

project(BetterFasterSpades)

#
# Add third party libraries
#

add_subdirectory(Extern)

#
# Add main target
#

add_executable(server "")

target_link_libraries(server
    PRIVATE
        enet
        zlib
)

add_subdirectory(Source)

#
# Add test
#

add_subdirectory(Test)
