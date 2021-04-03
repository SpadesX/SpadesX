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
add_compile_options(-Wall -Wextra)
add_executable(server "")

target_link_libraries(server
    PRIVATE
        enet
        z
)

add_subdirectory(Source)

#
# Add test
#

add_subdirectory(Test)
