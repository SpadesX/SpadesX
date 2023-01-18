#include <Server/Packets/Packets.h>
#include <Server/Server.h>
#include <Util/Checks/PositionChecks.h>
#include <Util/Enums.h>
#include <Util/Log.h>
#include <Util/Nanos.h>
#include <Util/Uthash.h>

// player_hit_id is the player that got shot
// player_id is the player who fired.
void receive_hit_packet(server_t* server, player_t* player, stream_t* data)
{
    uint8_t   hit_player_id = stream_read_u8(data);
    hit_t     hit_type      = stream_read_u8(data);
    player_t* hit_player;
    HASH_FIND(hh, server->players, &hit_player_id, sizeof(hit_player_id), hit_player);
    if (hit_player == NULL) {
        LOG_WARNING("Player %s (#%hhu) hit player who doesn't exist (#%hhu)", player->name, player->id, hit_player_id);
        return;
    }
    vector3f_t shot_pos     = player->movement.position;
    vector3f_t shot_eye_pos = player->movement.eye_pos;
    vector3f_t hit_pos      = hit_player->movement.position;
    vector3f_t shot_orien   = player->movement.forward_orientation;
    float      distance     = distance_in_2d(shot_pos, hit_pos);
    long       x = 0, y = 0, z = 0;

    if (player->sprinting || (player->item == TOOL_GUN && player->weapon_clip == 0)) {
        return; // Sprinting and hitting somebody is impossible
    }

    uint64_t timeNow = get_nanos();

    if (allow_shot(
        server, player, hit_player, timeNow, distance, &x, &y, &z, shot_pos, shot_orien, hit_pos, shot_eye_pos))
    {
        if(player->item == TOOL_GUN && player->weapon_pellets != 0) {
            player->weapon_pellets--;
        }

        switch (player->weapon) {
            case WEAPON_RIFLE:
            {
                switch (hit_type) {
                    case HIT_TYPE_HEAD:
                    {
                        send_kill_action_packet(server, player, hit_player, 1, 5, 0);
                        break;
                    }
                    case HIT_TYPE_TORSO:
                    {
                        send_set_hp(server, player, hit_player, 49, 1, 0, 5, 0, shot_pos);
                        break;
                    }
                    case HIT_TYPE_ARMS:
                    {
                        send_set_hp(server, player, hit_player, 33, 1, 0, 5, 0, shot_pos);
                        break;
                    }
                    case HIT_TYPE_LEGS:
                    {
                        send_set_hp(server, player, hit_player, 33, 1, 0, 5, 0, shot_pos);
                        break;
                    }
                    case HIT_TYPE_MELEE:
                        // Empty so we dont have errors :)
                        break;
                }
                break;
            }
            case WEAPON_SMG:
            {
                switch (hit_type) {
                    case HIT_TYPE_HEAD:
                    {
                        send_set_hp(server, player, hit_player, 75, 1, 1, 5, 0, shot_pos);
                        break;
                    }
                    case HIT_TYPE_TORSO:
                    {
                        send_set_hp(server, player, hit_player, 29, 1, 0, 5, 0, shot_pos);
                        break;
                    }
                    case HIT_TYPE_ARMS:
                    {
                        send_set_hp(server, player, hit_player, 18, 1, 0, 5, 0, shot_pos);
                        break;
                    }
                    case HIT_TYPE_LEGS:
                    {
                        send_set_hp(server, player, hit_player, 18, 1, 0, 5, 0, shot_pos);
                        break;
                    }
                    case HIT_TYPE_MELEE:
                        // Empty so we dont have errors :)
                        break;
                }
                break;
            }
            case WEAPON_SHOTGUN:
            {
                switch (hit_type) {
                    case HIT_TYPE_HEAD:
                    {
                        send_set_hp(server, player, hit_player, 37, 1, 1, 5, 0, shot_pos);
                        break;
                    }
                    case HIT_TYPE_TORSO:
                    {
                        send_set_hp(server, player, hit_player, 27, 1, 0, 5, 0, shot_pos);
                        break;
                    }
                    case HIT_TYPE_ARMS:
                    {
                        send_set_hp(server, player, hit_player, 16, 1, 0, 5, 0, shot_pos);
                        break;
                    }
                    case HIT_TYPE_LEGS:
                    {
                        send_set_hp(server, player, hit_player, 16, 1, 0, 5, 0, shot_pos);
                        break;
                    }
                    case HIT_TYPE_MELEE:
                        // Empty so we dont have errors :)
                        break;
                }
                break;
            }
        }
        if (hit_type == HIT_TYPE_MELEE) {
            send_set_hp(server, player, hit_player, 80, 1, 2, 5, 0, shot_pos);
        }
    }
}
