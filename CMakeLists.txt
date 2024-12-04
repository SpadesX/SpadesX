cmake_minimum_required(VERSION 3.16)

# SpadesX
project(SpadesX LANGUAGES C)

# Find pthread
find_package(Threads REQUIRED)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
option(CMAKE_UNITY_BUILD "Enable Unity Build" ON)

option(GIT_SUBMODULES_FETCH "Fetch the required Git submodules" ON)

# Add third party libraries
add_subdirectory(Extern)

# Add main target
add_compile_options(-Wall -Wextra -Werror -Wpedantic -Wno-error=unused-but-set-parameter -Wno-error=pedantic -std=gnu11 -fstack-protector-strong)
add_executable(SpadesX "")

target_link_libraries(SpadesX
    PRIVATE
        Server
        Util
        enet
        tomlc99
        mapvxl
        m
        json-c
        readline
        Threads::Threads
)

add_subdirectory(Source)

set_target_properties(SpadesX Util Server
  PROPERTIES
    INTERPROCEDURAL_OPTIMIZATION true
)

if (NOT EXISTS ${CMAKE_BINARY_DIR}/config.toml)
configure_file(${PROJECT_SOURCE_DIR}/Resources/config.toml ${CMAKE_BINARY_DIR}/config.toml COPYONLY)
endif()

if (NOT EXISTS ${CMAKE_BINARY_DIR}/Border_Hallway.vxl)
configure_file(${PROJECT_SOURCE_DIR}/Resources/Border_Hallway.vxl ${CMAKE_BINARY_DIR}/Border_Hallway.vxl COPYONLY)
endif()

if (NOT EXISTS ${CMAKE_BINARY_DIR}/Border_Hallway.toml)
configure_file(${PROJECT_SOURCE_DIR}/Resources/Border_Hallway.toml ${CMAKE_BINARY_DIR}/Border_Hallway.toml COPYONLY)
endif()
