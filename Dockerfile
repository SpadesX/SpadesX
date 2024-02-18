###################################
# Stage 1: Build-Time Environment #
###################################

FROM        alpine:latest AS build

RUN         apk add --no-cache \
            make \
            cmake \
            zlib-dev \
            json-c-dev \
            readline-dev \
            libbsd-dev

COPY        . /usr/src/spadesx

WORKDIR     /usr/src/spadesx

# utf-4096:
# enet only uses C, but since it doesn't specify "LANGUAGES C" then cmake checks for a
# C++ compiler.. so let's patch that out
RUN         sed -i 's/project(enet)/project(enet LANGUAGES C)/g' Extern/enet6/CMakeLists.txt

WORKDIR     /usr/src/spadesx/build

RUN         cmake ..; \
            make -j`nproc`; \
            mkdir /app; \
            cp SpadesX /app/SpadesX

################################
# Stage 2: Runtime Environment #
################################

FROM        scratch AS runtime

COPY        --from=build /app /app

VOLUME      [ "/app" ]

EXPOSE      32887/udp

CMD         [ "/app/SpadesX" ]
