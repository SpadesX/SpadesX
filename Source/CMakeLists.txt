#
# Add sources
#

set(SERVER_HEADERS
)

set(SERVER_SOURCES
    Server.c
)

target_sources(server
    PRIVATE
        ${SERVER_SOURCES}
        ${SERVER_HEADERS}
)
