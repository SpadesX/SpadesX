#include <Server/Packets/Packets.h>
#include "Util/DataStream.h"
#include "Util/Log.h"
#include "Util/Uthash.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

void receive_short_player(server_t* server, player_t* player, stream_t* data)
{
    stream_skip(data, 1); // Sender has to match the player we are getting info of.
    player->team   = stream_read_u8(data);
    player->weapon = stream_read_u8(data);

    if (player->team != 0 && player->team != 1 && player->team != 255) {
        LOG_WARNING("Player %s (#%hhu) sent invalid team. Switching them to Spectator", player->name, player->id);
        player->team = 255;
    }

    switch (player->weapon) {
        case 0:
            player->weapon_reserve  = 50;
            player->weapon_clip     = 10;
            player->default_clip    = RIFLE_DEFAULT_CLIP;
            player->default_reserve = RIFLE_DEFAULT_RESERVE;
            break;
        case 1:
            player->weapon_reserve  = 120;
            player->weapon_clip     = 30;
            player->default_clip    = SMG_DEFAULT_CLIP;
            player->default_reserve = SMG_DEFAULT_RESERVE;
            break;
        case 2:
            player->weapon_reserve  = 48;
            player->weapon_clip     = 6;
            player->default_clip    = SHOTGUN_DEFAULT_CLIP;
            player->default_reserve = SHOTGUN_DEFAULT_RESERVE;
            break;
    }

    if (server->protocol.gamemode.intel_held[0] == 0) {
        send_move_object(server, 0, 0, server->protocol.gamemode.intel[0]);
    }
    if (server->protocol.gamemode.intel_held[1] == 0) {
        send_move_object(server, 1, 1, server->protocol.gamemode.intel[1]);
    }
}
