#
# Add sources
#

set(SERVER_HEADERS
    Server.h
    Types.h
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

add_executable(readvxl ReadVXL.c)

target_link_libraries(readvxl PRIVATE zlib)
