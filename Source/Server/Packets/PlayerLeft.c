#include <Server/Events.h>
#include <Server/ParseConvert.h>
#include <Server/Server.h>
#include <Util/Checks/PlayerChecks.h>
#include <Util/Log.h>

void send_player_left(server_t* server, player_t* player)
{
    char ipString[17];
    format_ip_to_str(ipString, player->ip);
    LOG_INFO("Player %s (%s, #%hhu) disconnected", player->name, ipString, player->id);

    player_disconnect_run(player);

    if (server->protocol.num_players == 0) {
        return;
    }
    player_t *connected_player, *tmp;
    HASH_ITER(hh, server->players, connected_player, tmp)
    {
        if (connected_player->id != player->id && is_past_state_data(connected_player)) {
            ENetPacket* packet = enet_packet_create(NULL, 2, ENET_PACKET_FLAG_RELIABLE);
            stream_t    stream = {packet->data, packet->dataLength, 0};
            stream_write_u8(&stream, PACKET_TYPE_PLAYER_LEFT);
            stream_write_u8(&stream, player->id);

            if (enet_peer_send(connected_player->peer, 0, packet) != 0) {
                LOG_WARNING("Failed to send player left event");
                enet_packet_destroy(packet);
            }
        }
    }
}
