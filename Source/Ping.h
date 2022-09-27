// Copyright DarkNeutrino 2022
#ifndef PING_H
#define PING_H

#include <enet/enet.h>

int raw_udp_intercept_callback(ENetHost* host, ENetEvent* event);

#endif
