#include "Util/Enums.h"
#include <Server/Packets/Packets.h>
#include <Server/Packets/ReceivePackets.h>
#include <Util/Checks/TimeChecks.h>
#include <Util/Log.h>
#include <Util/Physics.h>
#include <Util/Uthash.h>
#include <Util/Utlist.h>

inline uint8_t allow_shot(server_t*  server,
                          player_t*  player,
                          player_t*  player_hit,
                          uint64_t   time_now,
                          float      distance,
                          long*      x,
                          long*      y,
                          long*      z,
                          vector3f_t shot_pos,
                          vector3f_t shot_orien,
                          vector3f_t hit_pos,
                          vector3f_t shot_eye_pos)
{
    uint8_t ret = 0;
    if (player->primary_fire &&
        ((player->item == TOOL_SPADE && diff_is_older_then(time_now, &player->timers.since_last_shot, NANO_IN_MILLI * 100)) ||
         (player->item == TOOL_GUN && player->weapon == WEAPON_RIFLE &&
          diff_is_older_then(time_now, &player->timers.since_last_shot, NANO_IN_MILLI * 500)) ||
         (player->item == TOOL_GUN && player->weapon == WEAPON_SMG &&
          diff_is_older_then(time_now, &player->timers.since_last_shot, NANO_IN_MILLI * 100)) ||
         (player->item == TOOL_GUN && player->weapon == WEAPON_SHOTGUN &&
          diff_is_older_then(time_now, &player->timers.since_last_shot, NANO_IN_MILLI * 1000))) &&
        player->alive && player_hit->alive && (player->team != player_hit->team || player->allow_team_killing) &&
        (player->allow_killing && server->global_ak) && physics_validate_hit(shot_pos, shot_orien, hit_pos, 5) &&
        (physics_cast_ray(server,
                          shot_eye_pos.x,
                          shot_eye_pos.y,
                          shot_eye_pos.z,
                          shot_orien.x,
                          shot_orien.y,
                          shot_orien.z,
                          distance,
                          x,
                          y,
                          z) == 0 ||
         physics_cast_ray(server,
                          shot_eye_pos.x,
                          shot_eye_pos.y,
                          shot_eye_pos.z - 1,
                          shot_orien.x,
                          shot_orien.y,
                          shot_orien.z,
                          distance,
                          x,
                          y,
                          z) == 0))
    {
        ret = 1;
    }
    return ret;
}

void init_packets(server_t* server)
{
    server->packets            = NULL;
    packet_manager_t packets[] = {{0, &receive_position_data},
                                  {1, &receive_orientation_data},
                                  {3, &receive_input_data},
                                  {4, &receive_weapon_input},
                                  {5, &receive_hit_packet},
                                  {6, &receive_grenade_packet},
                                  {7, &receive_set_tool},
                                  {8, &receive_set_color},
                                  {9, &receive_existing_player},
                                  {10, &receive_short_player},
                                  {13, &receive_block_action},
                                  {14, &receive_block_line},
                                  {17, &receive_handle_send_message},
                                  {28, &receive_weapon_reload},
                                  {29, &receive_change_team},
                                  {30, &receive_change_weapon},
                                  {34, &receive_version_response}};
    for (unsigned long i = 0; i < sizeof(packets) / sizeof(packet_manager_t); i++) {
        packet_t* packet = malloc(sizeof(packet_t));
        packet->id       = packets[i].id;
        packet->packet   = packets[i].packet;
        HASH_ADD_INT(server->packets, id, packet);
    }
}

void free_all_packets(server_t* server)
{
    packet_t* current_packet;
    packet_t* tmp;
    HASH_ITER(hh, server->packets, current_packet, tmp)
    {
        HASH_DEL(server->packets, current_packet);
        free(current_packet);
    }
}

void on_packet_received(server_t* server, player_t* player, stream_t* data)
{
    uint8_t   type     = stream_read_u8(data);
    packet_t* packet_p = NULL;
    int       type_int = (int) type;
    HASH_FIND_INT(server->packets, &type_int, packet_p);
    if (packet_p == NULL) {
        LOG_WARNING("Unknown packet with ID %d received", type_int);
        return;
    }
    packet_p->packet(server, player, data);
}
