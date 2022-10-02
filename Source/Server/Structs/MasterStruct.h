#ifndef MASTERSTRUCT_H
#define MASTERSTRUCT_H

#include <Util/Types.h>
#include <enet/enet.h>

typedef struct master
{
    // Master
    ENetHost* client;
    ENetPeer* peer;
    ENetEvent event;
    uint8_t   enable_master_connection;
    uint64_t  time_since_last_send;
} master_t;

#endif
