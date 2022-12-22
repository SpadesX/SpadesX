#include "Util/Enums.h"
#include "Util/Uthash.h"
#include <Server/Gamemodes/Gamemodes.h>
#include <Server/IntelTent.h>
#include <Server/Nodes.h>
#include <Server/Server.h>
#include <Server/Staff.h>
#include <Util/Checks/PlayerChecks.h>
#include <Util/Checks/PositionChecks.h>
#include <Util/Checks/TimeChecks.h>
#include <Util/Log.h>
#include <Util/Nanos.h>
#include <Util/Utlist.h>

void send_block_action(server_t* server, player_t* player, uint8_t actionType, int X, int Y, int Z)
{
    if (server->protocol.num_players == 0) {
        return;
    }
    ENetPacket* packet = enet_packet_create(NULL, 15, ENET_PACKET_FLAG_RELIABLE);
    stream_t    stream = {packet->data, packet->dataLength, 0};
    stream_write_u8(&stream, PACKET_TYPE_BLOCK_ACTION);
    stream_write_u8(&stream, player->id);
    stream_write_u8(&stream, actionType);
    stream_write_u32(&stream, X);
    stream_write_u32(&stream, Y);
    stream_write_u32(&stream, Z);
    uint8_t sent = 0;
    player_t *check, *tmp;
    HASH_ITER(hh, server->players, check, tmp) {
        if (is_past_state_data(check)) {
            if (enet_peer_send(player->peer, 0, packet) == 0) {
                sent = 1;
            }
        } else if (player->state == STATE_STARTING_MAP || player->state == STATE_LOADING_CHUNKS) {
            block_node_t* node = (block_node_t*) malloc(sizeof(*node));
            node->position.x   = X;
            node->position.y   = Y;
            node->position.z   = Z;
            node->color        = player->color;
            node->type         = actionType;
            node->sender_id    = player->id;
            LL_APPEND(player->blockBuffer, node);
        }
    }
    if (sent == 0) {
        enet_packet_destroy(packet);
    }
}
void send_block_action_to_player(server_t* server,
                                 player_t*   player,
                                 player_t*   receiver,
                                 uint8_t   actionType,
                                 int       X,
                                 int       Y,
                                 int       Z)
{
    if (server->protocol.num_players == 0) {
        return;
    }
    ENetPacket* packet = enet_packet_create(NULL, 15, ENET_PACKET_FLAG_RELIABLE);
    stream_t    stream = {packet->data, packet->dataLength, 0};
    stream_write_u8(&stream, PACKET_TYPE_BLOCK_ACTION);
    stream_write_u8(&stream, player->id);
    stream_write_u8(&stream, actionType);
    stream_write_u32(&stream, X);
    stream_write_u32(&stream, Y);
    stream_write_u32(&stream, Z);
    uint8_t sent = 0;
    if (enet_peer_send(receiver->peer, 0, packet) == 0) {
        sent = 1;
    }
    if (sent == 0) {
        enet_packet_destroy(packet);
    }
}

void receive_block_action(server_t* server, player_t* player, stream_t* data)
{
    uint8_t received_id = stream_read_u8(data);
    if (player->id != received_id) {
        LOG_WARNING("Assigned ID: %d doesnt match sent ID: %d in block action packet", player->id, received_id);
    }
    if (player->can_build && server->global_ab) {
        uint8_t  actionType = stream_read_u8(data);
        uint32_t X          = stream_read_u32(data);
        uint32_t Y          = stream_read_u32(data);
        uint32_t Z          = stream_read_u32(data);
        if (player->sprinting) {
            return;
        }
        vector3i_t vectorBlock  = {X, Y, Z};
        vector3f_t vectorfBlock = {(float) X, (float) Y, (float) Z};
        vector3f_t playerVector = player->movement.position;
        if (((player->item == 0 && (actionType == 1 || actionType == 2)) || (player->item == 1 && actionType == 0) ||
             (player->item == 2 && actionType == 1)))
        {
            if ((distance_in_3d(vectorfBlock, playerVector) <= 4 || player->item == 2) &&
                valid_pos_v3i(server, vectorBlock))
            {
                switch (actionType) {
                    case 0:
                    {
                        if (gamemode_block_checks(server, X, Y, Z)) {
                            uint64_t timeNow = get_nanos();
                            if (player->blocks > 0 &&
                                diff_is_older_then(timeNow, &player->timers.since_last_block_plac, BLOCK_DELAY) &&
                                diff_is_older_then_dont_update(
                                timeNow, player->timers.since_last_block_dest, BLOCK_DELAY) &&
                                diff_is_older_then_dont_update(
                                timeNow, player->timers.since_last_3block_dest, BLOCK_DELAY))
                            {
                                mapvxl_set_color(&server->s_map.map, X, Y, Z, player->tool_color.raw);
                                player->blocks--;
                                moveIntelAndTentUp(server);
                                send_block_action(server, player, actionType, X, Y, Z);
                            }
                        }
                    } break;

                    case 1:
                    {
                        if (Z < 62 && gamemode_block_checks(server, X, Y, Z)) {
                            uint64_t timeNow = get_nanos();
                            if ((diff_is_older_then(timeNow, &player->timers.since_last_block_dest, SPADE_DELAY) &&
                                 diff_is_older_then_dont_update(
                                 timeNow, player->timers.since_last_3block_dest, SPADE_DELAY) &&
                                 diff_is_older_then_dont_update(
                                 timeNow, player->timers.since_last_block_plac, SPADE_DELAY)) ||
                                player->item == 2)
                            {
                                if (player->item == 2) {
                                    
                                    if (player->weapon_clip <= 0 && !diff_is_older_then_dont_update(timeNow, player->timers.since_last_primary_weapon_input, 10*NANO_IN_MILLI)) {
                                        send_message_to_staff(server,
                                                              "Player %s (#%hhu) probably has hack to have more ammo",
                                                              player->name,
                                                              player->id);
                                        return;
                                    } else if (player->weapon == 0 &&
                                               diff_is_older_then(timeNow,
                                                                  &player->timers.since_last_block_dest_with_gun,
                                                                  ((RIFLE_DELAY * 2) - (NANO_IN_MILLI * 10))) == 0)
                                    {
                                        send_message_to_staff(server,
                                                              "Player %s (#%hhu) probably has rapid shooting hack",
                                                              player->name,
                                                              player->id);
                                        return;
                                    } else if (player->weapon == 1 &&
                                               diff_is_older_then(timeNow,
                                                                  &player->timers.since_last_block_dest_with_gun,
                                                                  ((SMG_DELAY * 3) - (NANO_IN_MILLI * 10))) == 0)
                                    {
                                        send_message_to_staff(server,
                                                              "Player %s (#%hhu) probably has rapid shooting hack",
                                                              player->name,
                                                              player->id);
                                        return;
                                    } else if (player->weapon == 2 &&
                                               diff_is_older_then(timeNow,
                                                                  &player->timers.since_last_block_dest_with_gun,
                                                                  (SHOTGUN_DELAY - (NANO_IN_MILLI * 10))) == 0)
                                    {
                                        send_message_to_staff(server,
                                                              "Player %s (#%hhu) probably has rapid shooting hack",
                                                              player->name,
                                                              player->id);
                                        return;
                                    }
                                }

                                vector3i_t  position = {X, Y, Z};
                                vector3i_t* neigh    = get_neighbours(position);
                                mapvxl_set_air(&server->s_map.map, position.x, position.y, position.z);
                                for (int i = 0; i < 6; ++i) {
                                    if (neigh[i].z < 62) {
                                        check_node(server, neigh[i]);
                                    }
                                }
                                if (player->item != 2) {
                                    if (player->blocks < 50) {
                                        player->blocks++;
                                    }
                                }
                                send_block_action(server, player, actionType, X, Y, Z);
                            }
                        }
                    } break;

                    case 2:
                    {
                        if (gamemode_block_checks(server, X, Y, Z) && gamemode_block_checks(server, X, Y, Z + 1) &&
                            gamemode_block_checks(server, X, Y, Z - 1))
                        {
                            uint64_t timeNow = get_nanos();
                            if (diff_is_older_then(timeNow, &player->timers.since_last_3block_dest, THREEBLOCK_DELAY) &&
                                diff_is_older_then_dont_update(
                                timeNow, player->timers.since_last_block_dest, THREEBLOCK_DELAY) &&
                                diff_is_older_then_dont_update(
                                timeNow, player->timers.since_last_block_plac, THREEBLOCK_DELAY))
                            {
                                for (uint32_t z = Z - 1; z <= Z + 1; z++) {
                                    if (z < 62) {
                                        mapvxl_set_air(&server->s_map.map, X, Y, z);
                                        vector3i_t  position = {X, Y, z};
                                        vector3i_t* neigh    = get_neighbours(position);
                                        mapvxl_set_air(&server->s_map.map, position.x, position.y, position.z);
                                        for (int i = 0; i < 6; ++i) {
                                            if (neigh[i].z < 62) {
                                                check_node(server, neigh[i]);
                                            }
                                        }
                                    }
                                }
                                send_block_action(server, player, actionType, X, Y, Z);
                            }
                        }
                    } break;
                }
                move_intel_and_tent_down(server);
            }
        } else {
            LOG_WARNING("Player %s (#%hhu) may be using BlockExploit with Item: %d and Action: %d",
                        player->name,
                        player->id,
                        player->item,
                        actionType);
        }
    }
}
