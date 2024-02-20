###################################
# Stage 1: Build-Time Environment #
###################################

FROM        debian:testing AS build

RUN         apt-get -y update; \
            apt-get -y install \
            libbsd-dev \
            build-essential \
            cmake \
            zlib1g-dev \
            libjson-c-dev \
            libreadline-dev \
            git

COPY        . /usr/src/spadesx

WORKDIR     /usr/src/spadesx/build

RUN         cmake ..; \
            make -j`nproc`; \
            mkdir /app; \
            cp SpadesX /app/

################################
# Stage 2: Runtime Environment #
################################

FROM        scratch AS runtime

COPY        --from=build /app /app

VOLUME      [ "/app" ]

EXPOSE      32887/udp

CMD         [ "/app/SpadesX" ]
