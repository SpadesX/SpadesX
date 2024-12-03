# Build SpadesX in separate image 'build'
FROM    alpine:edge AS build

# Install packages for building;
# create output dir;
# <sys/cdefs.h> is deprecated, this is a workaround until it gets fixed
RUN     apk add --no-cache \
        build-base \
        cmake \
        gcc \
        json-c-dev \
        libc-dev \
        make \
        musl-dev \
        ncurses-static \
        readline-dev \
        readline-static \
        zlib-dev \
        zlib-static; \
        mkdir /app

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
