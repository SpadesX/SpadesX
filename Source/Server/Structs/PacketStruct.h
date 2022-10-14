#ifndef PACKETSTRUCT_H
#define PACKETSTRUCT_H

#include <Util/DataStream.h>
#include <Util/Types.h>
#include <Util/Uthash.h>

typedef struct server server_t;

typedef struct packet
{
    int id;
    void (*packet)(server_t* server, uint8_t playerID, stream_t* data);
    UT_hash_handle hh;
} packet_t;

typedef struct packet_manager
{
    int id;
    void (*packet)(server_t* server, uint8_t playerID, stream_t* data);
} packet_manager_t;

#endif