#include <Server/Events.h>
#include <Server/ParseConvert.h>
#include <Server/Server.h>
#include <Util/Checks/PlayerChecks.h>
#include <Util/Log.h>

void send_player_left(server_t* server, uint8_t player_id)
{
    char      ipString[17];
    player_t* player = &server->player[player_id];
    format_ip_to_str(ipString, player->ip);
    LOG_INFO("Player %s (%s, #%hhu) disconnected", player->name, ipString, player_id);

    player_disconnect_run(player_id);

    if (server->protocol.num_players == 0) {
        return;
    }
    for (uint8_t i = 0; i < server->protocol.max_players; ++i) {
        if (i != player_id && is_past_state_data(server, i)) {
            ENetPacket* packet = enet_packet_create(NULL, 2, ENET_PACKET_FLAG_RELIABLE);
            stream_t    stream = {packet->data, packet->dataLength, 0};
            stream_write_u8(&stream, PACKET_TYPE_PLAYER_LEFT);
            stream_write_u8(&stream, player_id);

            if (enet_peer_send(server->player[i].peer, 0, packet) != 0) {
                LOG_WARNING("Failed to send player left event");
                enet_packet_destroy(packet);
            }
        }
    }
}
