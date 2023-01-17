#ifndef PACKETSTRUCT_H
#define PACKETSTRUCT_H

#include <Server/Structs/PlayerStruct.h>
#include <Util/DataStream.h>
#include <Util/Types.h>
#include <Util/Uthash.h>

typedef struct server server_t;

typedef struct packet
{
    int id;
    void (*packet)(server_t* server, player_t* player, stream_t* data);
    UT_hash_handle hh;
} packet_t;

typedef struct packet_manager
{
    int id;
    void (*packet)(server_t* server, player_t* player, stream_t* data);
} packet_manager_t;

#endif
