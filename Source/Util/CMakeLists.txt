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
    if (NOT "${VCPKG_INSTALLED_DIR}" STREQUAL "")
        if("${CMAKE_C_COMPILER_ID}" STREQUAL "MSVC")
            find_package(unofficial-enet CONFIG REQUIRED)
            find_package(ZLIB REQUIRED)

            target_link_libraries(util
                PRIVATE
                    ZLIB::ZLIB
                    unofficial::enet::enet
            )
        else()
            message(FATAL_ERROR "Sorry, but compiling without MSVC on Windows is not yet supported.")
        endif()
    else()
        message(FATAL_ERROR "Sorry, but compiling using CMake on Windows without VCPKG is not yet supported.")
    endif()
else()
    target_link_libraries(util
        PRIVATE
            z # zlib
    )
endif()