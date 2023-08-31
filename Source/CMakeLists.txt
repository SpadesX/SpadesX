#
# Add sources
#

set(SPADESX_SOURCES
    Main.c
)

target_sources(SpadesX
    PRIVATE
        ${SPADESX_SOURCES}
)

target_compile_features(SpadesX
    PRIVATE
        c_std_11
)

add_library(SpadesXCommon INTERFACE)

target_include_directories(SpadesXCommon
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${PROJECT_SOURCE_DIR}/Extern
        ${PROJECT_SOURCE_DIR}/Extern/spadesx_enet/include
        ${LUA_INCLUDE_DIR}
)

add_subdirectory(Util)
add_subdirectory(Server)

target_link_libraries(Server
    PRIVATE
        lua5.4
        SpadesXCommon
        Util
        mapvxl
)
