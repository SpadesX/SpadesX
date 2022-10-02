#ifndef PROTOCOLSTRUCT_H
#define PROTOCOLSTRUCT_H

#include <Server/Structs/GamemodeStruct.h>
#include <Util/Types.h>

typedef struct protocol
{
    uint8_t num_users;
    uint8_t num_team_users[2];
    //
    uint8_t num_players;
    uint8_t max_players;
    //
    color_t         color_fog;
    color_t         color_team[2];
    char            name_team[2][11];
    gamemode_t      current_gamemode;
    gamemode_vars_t gamemode;
    // respawn area
    quad3d_t spawns[2];
    uint32_t input_flags;
} protocol_t;

#endif
