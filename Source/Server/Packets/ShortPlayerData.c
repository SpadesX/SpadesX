#include <Server/Packets/Packets.h>
#include <Server/ParseConvert.h>
#include <Util/DataStream.h>
#include <Util/Enums.h>
#include <Util/Log.h>
#include <Util/Uthash.h>
#include <Util/Weapon.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

void receive_short_player(server_t* server, player_t* player, stream_t* data)
{
    if (player->team != 2) {
        return;
    }
    stream_skip(data, 1); // Sender has to match the player we are getting info of.
    player->team   = stream_read_u8(data);
    player->weapon = stream_read_u8(data);

    if (player->team != 0 && player->team != 1 && player->team != 2) {
        LOG_WARNING("Player %s (#%hhu) sent invalid team. Switching them to Spectator", player->name, player->id);
        player->team = 2;
    }

    set_default_player_ammo(player);

    player->state = STATE_SPAWNING;
    char IP[17];
    format_ip_to_str(IP, player->ip);
    char team[15];
    team_id_to_str(server, team, player->team);
    LOG_INFO("Player %s (%s, #%hhu) joined %s", player->name, IP, player->id, team);
}
