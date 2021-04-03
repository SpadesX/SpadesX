#
# Add sources
#

add_compile_options(-Wall -Wextra)

set(SERVER_HEADERS
    Queue.h
    Compress.h
    DataStream.h
    Server.h
    Types.h
)

set(SERVER_SOURCES
    Queue.c
    Compress.c
    DataStream.c
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

add_executable(readvxl ReadVXL.c)

target_link_libraries(readvxl PRIVATE z)
