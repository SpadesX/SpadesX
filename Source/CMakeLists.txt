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
    PacketReceive.h
    Conversion.h
    ColorConversion.h
    Commands.h
)

set(SERVER_SOURCES
    Server.c
    Main.c
    Master.c
    Map.c
    Protocol.c
    Packets.c
    PacketReceive.c
    Commands.c
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
