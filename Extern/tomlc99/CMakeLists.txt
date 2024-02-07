cmake_minimum_required(VERSION 3.16)

project(tomlc99 LANGUAGES C)

add_library(tomlc99 STATIC)

target_sources(tomlc99
    PRIVATE
        "tomlc99/toml.h"
        "tomlc99/toml.c"
)

target_compile_features(tomlc99
    PUBLIC
        c_std_99
)

target_compile_options(tomlc99
    PRIVATE
        # Add Warnings and Optimization Levels (do not add for MSVC)
        $<$<NOT:$<C_COMPILER_ID:MSVC>>:-Wall -Wextra $<IF:$<CONFIG:Debug>,-Og,-O2>>
)

target_include_directories(tomlc99
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)
