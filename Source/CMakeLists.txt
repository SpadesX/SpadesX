#
# Add sources
#

set(SERVER_HEADERS
    Player.h
    Protocol.h
    Enums.h
    Server.h
)

set(SERVER_SOURCES
    Server.c
    Main.c
)

target_sources(server
    PRIVATE
        ${SERVER_SOURCES}
        ${SERVER_HEADERS}
)

target_compile_features(server
    PRIVATE
        c_std_11
)

target_include_directories(server
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${PROJECT_SOURCE_DIR}/Extern
)

add_subdirectory(Util)
