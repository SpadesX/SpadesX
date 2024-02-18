###################################
# Stage 1: Build-Time Environment #
###################################

FROM        alpine:latest AS build

RUN         set -ex; \
            apk add --no-cache \
            build-base \
            cmake \
            zlib-dev \
            json-c-dev \
            readline-dev \
            libbsd-dev \
            git

COPY        . /usr/src/spadesx

WORKDIR     /usr/src/spadesx

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
