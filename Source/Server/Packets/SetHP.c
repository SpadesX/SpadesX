#include <Server/Packets/Packets.h>
#include <Server/Server.h>

void send_set_hp(server_t*  server,
                 player_t*  player,
                 player_t*  player_hit,
                 long       hp_chnage,
                 uint8_t    type_of_damage,
                 uint8_t    kill_reason,
                 uint8_t    respawn_time,
                 uint8_t    is_grenade,
                 vector3f_t position)
{
    if (server->protocol.num_players == 0 || player_hit->team == TEAM_SPECTATOR ||
        (!player->allow_team_killing && player->team == player_hit->team && player->id != player_hit->id))
    {
        return;
    }
    if ((player->allow_killing && server->global_ak && player->allow_killing && player->alive) || type_of_damage == 0) {
        if (hp_chnage > player_hit->hp) {
            hp_chnage = player_hit->hp;
        }
        player_hit->hp -= hp_chnage;
        if (player_hit->hp < 0) // We should NEVER return true here. If we do stuff is really broken
            player_hit->hp = 0;

        else if (player_hit->hp > 100) // Same as above
            player_hit->hp = 100;

        if (player_hit->hp == 0) {
            send_kill_action_packet(server, player, player_hit, kill_reason, respawn_time, 0);
            return;
        }

        else if (player_hit->hp > 0 && player_hit->hp < 100)
        {
            ENetPacket* packet = enet_packet_create(NULL, 15, ENET_PACKET_FLAG_RELIABLE);
            stream_t    stream = {packet->data, packet->dataLength, 0};
            stream_write_u8(&stream, PACKET_TYPE_SET_HP);
            stream_write_u8(&stream, player_hit->hp);
            stream_write_u8(&stream, type_of_damage);
            if (type_of_damage != 0 && is_grenade == 0) {
                stream_write_f(&stream, player->movement.position.x);
                stream_write_f(&stream, player->movement.position.y);
                stream_write_f(&stream, player->movement.position.z);
            } else if (type_of_damage != 0 && is_grenade == 1) {
                stream_write_f(&stream, position.x);
                stream_write_f(&stream, position.y);
                stream_write_f(&stream, position.z);
            } else {
                stream_write_f(&stream, 0);
                stream_write_f(&stream, 0);
                stream_write_f(&stream, 0);
            }
            if (enet_peer_send(player_hit->peer, 0, packet) != 0) {
                enet_packet_destroy(packet);
            }
        }
    }
}
