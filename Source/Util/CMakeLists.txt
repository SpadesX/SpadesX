#
# Add sources
#

add_library(util STATIC "")

set(UTIL_HEADERS
    Queue.h
    Compress.h
    DataStream.h
    Types.h
    Physics.h
    Line.h
    Functions.h)

set(UTIL_SOURCES
    Queue.c
    Compress.c
    DataStream.c
    Physics.c
    Line.c
    Functions.c)

target_sources(util
    PRIVATE
        ${UTIL_SOURCES}
        ${UTIL_HEADERS}
 )

target_include_directories(util
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

if(WIN32)
    find_package(unofficial-enet CONFIG REQUIRED)
    find_package(ZLIB REQUIRED)

    target_link_libraries(util
        PRIVATE
            ZLIB::ZLIB
            unofficial::enet::enet
    )
else()
    target_link_libraries(util
        PRIVATE
            z # zlib
    )
endif()