#include <Server/Server.h>
#include <Util/Checks/PacketChecks.h>
#include <Util/Log.h>

void send_input_data(server_t* server, player_t* player)
{
    if (server->protocol.num_players == 0) {
        return;
    }
    ENetPacket* packet = enet_packet_create(NULL, 3, ENET_PACKET_FLAG_RELIABLE);
    stream_t    stream = {packet->data, packet->dataLength, 0};
    stream_write_u8(&stream, PACKET_TYPE_INPUT_DATA);
    stream_write_u8(&stream, player->id);
    stream_write_u8(&stream, player->input);
    if (send_packet_except_sender_dist_check(server, packet, player) == 0) {
        enet_packet_destroy(packet);
    }
}

void receive_input_data(server_t* server, player_t* player, stream_t* data)
{
    uint8_t bits[8];
    uint8_t mask        = 1;
    uint8_t received_id = stream_read_u8(data);
    if (player->id != received_id) {
        LOG_WARNING("Assigned ID: %d doesnt match sent ID: %d in Input packet", player->id, received_id);
    } else if (player->state == STATE_READY) {
        player->input = stream_read_u8(data);
        for (int i = 0; i < 8; i++) {
            bits[i] = (player->input >> i) & mask;
        }
        player->move_forward   = bits[0];
        player->move_backwards = bits[1];
        player->move_left      = bits[2];
        player->move_right     = bits[3];
        if (player->movement.velocity.z == 0.0f) {
            player->jumping = bits[4];
        }
        else {
            player->jumping = 0;
        }
        player->crouching = bits[5];
        player->sneaking  = bits[6];
        player->sprinting = bits[7];
        LOG_STATUS("velocity Z = %f and bit is %d", player->movement.velocity.z, bits[4]);
        send_input_data(server, player);
    }
}
