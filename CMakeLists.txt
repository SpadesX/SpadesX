cmake_minimum_required(VERSION 3.18)

#
# Project1
#

project(Project1)

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
)

add_subdirectory(Source)

#
# Add test
#

add_subdirectory(Test)
