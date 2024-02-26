# Build SpadesX in separate image 'build'
FROM    alpine:edge AS build

# Install packages for building;
# create output dir;
# <sys/cdefs.h> is deprecated, this is a workaround until it gets fixed
RUN     apk add --no-cache \
        gcc \
        cmake \
        make \
        musl-dev \
        libc-dev \
        json-c-dev \
        readline-dev \
        readline-static \
        ncurses-static \
        libbsd-dev \
        libbsd-static \
        zlib-dev \
        zlib-static; \
        mkdir /app; \
        sed -i 's/\#include <bsd\/sys\/cdefs.h>/\#define __BEGIN_DECLS\n\#define __END_DECLS\n\#define __GLIBC_PREREQ(x,y) 0/' /usr/include/bsd/string.h

COPY    . /usr/src/spadesx

WORKDIR /usr/src/spadesx

# readline relies on ncurses on alpine
RUN     sed -i 's/readline/readline ncurses/' CMakeLists.txt; \
        mkdir build; \
        cp Resources/* /app

WORKDIR /usr/src/spadesx/build

RUN     CFLAGS="-static" cmake .. -DGIT_SUBMODULES_FETCH=OFF; \
        make -j$(nproc); \
        cp SpadesX /app

# Runtime image uses built app, only scratch base is required
FROM    scratch AS runtime

# Copy built files
COPY    --from=build /app /app

WORKDIR /app

# Expose ports used by SpadesX
EXPOSE  32887/udp

# Run SpadesX
CMD     ["/app/SpadesX"]
