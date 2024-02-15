FROM        debian:bookworm

VOLUME      ["/server"]

WORKDIR     /usr/src/SpadesX

RUN         set -ex; \
            apt -y update; \
            apt -y upgrade; \
            apt install build-essential make cmake zlib1g-dev libjson-c-dev libreadline-dev libbsd-dev git; \
            git clone https://github.com/SpadesX/SpadesX.git; \
            cd SpadesX; \
            mkdir build; \
            cd build; \
            cmake ..; \
            make; \
            cd ..; \
            mkdir /server; \
            cp build/SpadesX /server; \
            cp -a Resources/. /server

WORKDIR     /server

EXPOSE      32887

ENTRYPOINT  [ "/server/SpadesX" ]
