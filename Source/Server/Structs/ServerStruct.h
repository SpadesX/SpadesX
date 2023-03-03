#ifndef SERVERSTRUCT_H
#define SERVERSTRUCT_H

#include <Server/Structs/MasterStruct.h>
#include <Server/Structs/PacketStruct.h>
#include <Server/Structs/PhysicsStruct.h>
#include <Server/Structs/PlayerStruct.h>
#include <Server/Structs/ProtocolStruct.h>
#include <Server/Structs/TimerStruct.h>
#include <Util/MersenneTwister/MT.h>
#include <Util/Types.h>
#include <signal.h>

typedef struct server
{
    ENetHost*             host;
    player_t*             players;
    protocol_t            protocol;
    master_t              master;
    packet_t*             packets;
    physics_t             physics;
    mt_rand_t             rand;
    uint16_t              port;
    map_t                 s_map;
    global_timers_t       global_timers;
    command_t*            cmds_map;
    command_t*            cmds_list;
    string_node_t*        welcome_messages;
    uint8_t               welcome_messages_count;
    string_node_t*        periodic_messages;
    uint8_t               periodic_message_count;
    uint8_t*              periodic_delays;
    uint8_t               global_ak;
    uint8_t               global_ab;
    const char*           manager_passwd;
    const char*           admin_passwd;
    const char*           mod_passwd;
    const char*           guard_passwd;
    const char*           trusted_passwd;
    char                  map_name[20];
    char                  server_name[31];
    char                  gamemode_name[7];
    volatile sig_atomic_t running; // volatile keyword is required to have an access to this variable in any thread
} server_t;

#endif
