#include "Util/Weapon.h"

#include <Server/Server.h>
#include <Server/Staff.h>
#include <Util/Checks/PacketChecks.h>
#include <Util/Checks/PlayerChecks.h>
#include <Util/Checks/TimeChecks.h>
#include <Util/Enums.h>
#include <Util/Log.h>
#include <Util/Nanos.h>
#include <Util/Notice.h>
#include <stdio.h>

void send_weapon_input(server_t* server, player_t* player, uint8_t wInput)
{
    if (server->protocol.num_players == 0) {
        return;
    }
    ENetPacket* packet = enet_packet_create(NULL, 3, ENET_PACKET_FLAG_RELIABLE);
    stream_t    stream = {packet->data, packet->dataLength, 0};
    stream_write_u8(&stream, PACKET_TYPE_WEAPON_INPUT);
    stream_write_u8(&stream, player->id);
    if (player->sprinting) {
        wInput = 0;
    }
    stream_write_u8(&stream, wInput);
    if (send_packet_except_sender(server, packet, player) == 0) {
        enet_packet_destroy(packet);
    }
}

void receive_weapon_input(server_t* server, player_t* player, stream_t* data)
{
    uint8_t mask           = 1;
    uint8_t received_id    = stream_read_u8(data);
    uint8_t received_input = stream_read_u8(data);
    if (player->id != received_id) {
        LOG_WARNING("Assigned ID: %d doesnt match sent ID: %d in weapon input packet", player->id, received_id);
    } else if (player->state != STATE_READY) {
        return;
    } else if (player->sprinting) {
        received_input = 0; /* Do not return just set it to 0 as we want to send to players that the player is no longer
                    shooting when they start sprinting */
    }
    player->primary_fire   = received_input & mask;
    player->secondary_fire = (received_input >> 1) & mask;

    if (player->secondary_fire && player->item == TOOL_BLOCK) {
        player->locAtClick = player->movement.position;
    } else if (player->secondary_fire && player->item == TOOL_SPADE) {
        player->timers.since_possible_spade_nade = get_nanos();
    }

    else if (player->weapon_clip > 0 && player->item == TOOL_GUN)
    {
        send_weapon_input(server, player, received_input);
        uint64_t timeDiff = get_player_weapon_delay_nano(player);

        if (player->primary_fire && diff_is_older_then(get_nanos(), &player->timers.since_last_weapon_input, timeDiff))
        {
            player->timers.since_last_weapon_input = get_nanos();
            player->weapon_clip--;
            if (player->weapon_clip == 0) {
                player->primary_fire   = 0;
                player->secondary_fire = 0;
                send_weapon_input(server, player, 0);
            }
            player->reloading = 0;
            if ((player->movement.previous_orientation.x == player->movement.forward_orientation.x) &&
                (player->movement.previous_orientation.y == player->movement.forward_orientation.y) &&
                (player->movement.previous_orientation.z == player->movement.forward_orientation.z) &&
                player->item == TOOL_GUN)
            {
                player_t *connected_player, *tmp;
                HASH_ITER(hh, server->players, connected_player, tmp)
                {
                    if (is_past_join_screen(connected_player) && is_staff(server, connected_player)) {
                        char message[200];
                        snprintf(message, 200, "WARNING. Player %d may be using no recoil", player->id);
                        send_server_notice(connected_player, 0, message);
                    }
                }
            }
            player->movement.previous_orientation = player->movement.forward_orientation;
        }
    } else {
        // sendKillActionPacket(server, player_id, player_id, 0, 30, 0);
    }
}
