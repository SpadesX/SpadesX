#include <Server/Server.h>
#include <Util/Checks/PlayerChecks.h>
#include <Util/Log.h>
#include <Util/Uthash.h>

void send_create_player(server_t* server, player_t* receiver, player_t* child)
{
    if (server->protocol.num_players == 0) {
        return;
    }
    ENetPacket* packet = enet_packet_create(NULL, 32, ENET_PACKET_FLAG_RELIABLE);
    stream_t    stream = {packet->data, packet->dataLength, 0};
    stream_write_u8(&stream, PACKET_TYPE_CREATE_PLAYER);
    stream_write_u8(&stream, child->id);                      // ID
    stream_write_u8(&stream, child->weapon);                  // WEAPON
    stream_write_u8(&stream, child->team);                    // TEAM
    stream_write_vector3f(&stream, child->movement.position); // X Y Z
    stream_write_array(&stream, child->name, 16);             // NAME

    if (enet_peer_send(receiver->peer, 0, packet) != 0) {
        LOG_WARNING("Failed to send player state");
        enet_packet_destroy(packet);
    }
}

void send_respawn(server_t* server, player_t* respawn_player)
{
    player_t *player, *tmp;
    HASH_ITER(hh, server->players, player, tmp)
    {
        if (is_past_state_data(player)) {
            send_create_player(server, player, respawn_player);
        }
    }
    respawn_player->state = STATE_READY;
}
