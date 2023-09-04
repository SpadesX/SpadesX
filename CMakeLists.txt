cmake_minimum_required(VERSION 3.16)

# SpadesX
project(SpadesX)

# Find pthread
find_package(Threads REQUIRED)

# Find lua
find_package(Lua REQUIRED)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
option(CMAKE_UNITY_BUILD "Enable Unity Build" ON)

# Add third party libraries
add_subdirectory(Extern)

# Add main target
add_compile_options(-Wall -Wextra -Werror -Wpedantic -Wno-error=unused-but-set-parameter -Wno-error=pedantic -std=gnu11 -fstack-protector-strong)
add_executable(SpadesX "")

target_link_libraries(SpadesX
    PRIVATE
        lua5.4
        Server
        Util
        enet
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

if (NOT EXISTS ${CMAKE_BINARY_DIR}/GameMode.lua)
configure_file(${PROJECT_SOURCE_DIR}/Resources/GameMode.lua ${CMAKE_BINARY_DIR}/GameMode.lua COPYONLY)
endif()

if (NOT EXISTS ${CMAKE_BINARY_DIR}/Border_Hallway.vxl)
configure_file(${PROJECT_SOURCE_DIR}/Resources/Border_Hallway.vxl ${CMAKE_BINARY_DIR}/Border_Hallway.vxl COPYONLY)
endif()

if (NOT EXISTS ${CMAKE_BINARY_DIR}/Border_Hallway.json)
configure_file(${PROJECT_SOURCE_DIR}/Resources/Border_Hallway.json ${CMAKE_BINARY_DIR}/Border_Hallway.json COPYONLY)
endif()
