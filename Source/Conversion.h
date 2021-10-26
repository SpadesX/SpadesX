// Copyright DarkNeutrino 2021
#ifndef CONVERSION_H
#define CONVERSION_H

#include "Structs.h"
#include <Types.h>

#include <string.h>
#include <enet/enet.h>

uint8* uint32ToUint8(uint32 normal)
{
    union {
        uint8 ip[4];
        uint32 ip32;
    } tempIpUnion;
    tempIpUnion.ip32                      = normal;
    uint8 *ip = malloc(sizeof(*ip) / sizeof(uint8));
    memcpy(ip, tempIpUnion.ip, sizeof(*ip) / sizeof(uint8));
    return ip;
}

#endif
