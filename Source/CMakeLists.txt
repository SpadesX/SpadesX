#
# Add sources
#

set(SERVER_HEADERS
    Protocol.h
    Master.h
    Enums.h
    Server.h
    Structs.h
    Map.h
    Packets.h
    Conversion.h
    Commands.h
    Gamemodes.h
)

set(SERVER_SOURCES
    Server.c
    Main.c
    Master.c
    Map.c
    Protocol.c
    Packets.c
    Commands.c
    Gamemodes.c
)

target_sources(SpadesX
    PRIVATE
        ${SERVER_SOURCES}
        ${SERVER_HEADERS}
)

target_compile_features(SpadesX
    PRIVATE
        c_std_11
)

target_include_directories(SpadesX
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${PROJECT_SOURCE_DIR}/Extern
)

add_subdirectory(Util)
