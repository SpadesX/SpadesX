#include <Server/Server.h>
#include <Util/Checks/PlayerChecks.h>
#include <Util/Log.h>

void send_create_player(server_t* server, uint8_t player_id, uint8_t other_id)
{
    if (server->protocol.num_players == 0) {
        return;
    }
    ENetPacket* packet       = enet_packet_create(NULL, 32, ENET_PACKET_FLAG_RELIABLE);
    stream_t    stream       = {packet->data, packet->dataLength, 0};
    player_t*   player       = &server->player[player_id];
    player_t*   player_other = &server->player[other_id];
    stream_write_u8(&stream, PACKET_TYPE_CREATE_PLAYER);
    stream_write_u8(&stream, other_id);                              // ID
    stream_write_u8(&stream, player_other->weapon);                  // WEAPON
    stream_write_u8(&stream, player_other->team);                    // TEAM
    stream_write_vector3f(&stream, player_other->movement.position); // X Y Z
    stream_write_array(&stream, player_other->name, 16);             // NAME

    if (enet_peer_send(player->peer, 0, packet) != 0) {
        LOG_WARNING("Failed to send player state");
        enet_packet_destroy(packet);
    }
}

void send_respawn(server_t* server, uint8_t player_id)
{
    player_t* player = &server->player[player_id];
    for (uint8_t i = 0; i < server->protocol.max_players; ++i) {
        if (is_past_join_screen(server, i)) {
            send_create_player(server, i, player_id);
        }
    }
    player->state = STATE_READY;
}
