#include <Server/Packets/Packets.h>
#include <Server/Server.h>
#include <Util/Checks/PositionChecks.h>
#include <Util/Nanos.h>

// player_hit_id is the player that got shot
// player_id is the player who fired.
void receive_hit_packet(server_t* server, uint8_t player_id, stream_t* data)
{
    player_t*  player        = &server->player[player_id];
    uint8_t    hit_player_id = stream_read_u8(data);
    hit_t      hit_type      = stream_read_u8(data);
    vector3f_t shot_pos      = player->movement.position;
    vector3f_t shot_eye_pos  = player->movement.eye_pos;
    vector3f_t hit_pos       = server->player[hit_player_id].movement.position;
    vector3f_t shot_orien    = player->movement.forward_orientation;
    float      distance      = distance_in_2d(shot_pos, hit_pos);
    long       x = 0, y = 0, z = 0;

    if (player->sprinting || (player->item == 2 && player->weapon_clip <= 0)) {
        return; // Sprinting and hitting somebody is impossible
    }

    uint64_t timeNow = get_nanos();

    if (allow_shot(
        server, player_id, hit_player_id, timeNow, distance, &x, &y, &z, shot_pos, shot_orien, hit_pos, shot_eye_pos))
    {
        switch (player->weapon) {
            case WEAPON_RIFLE:
            {
                switch (hit_type) {
                    case HIT_TYPE_HEAD:
                    {
                        send_kill_action_packet(server, player_id, hit_player_id, 1, 5, 0);
                        break;
                    }
                    case HIT_TYPE_TORSO:
                    {
                        send_set_hp(server, player_id, hit_player_id, 49, 1, 0, 5, 0, shot_pos);
                        break;
                    }
                    case HIT_TYPE_ARMS:
                    {
                        send_set_hp(server, player_id, hit_player_id, 33, 1, 0, 5, 0, shot_pos);
                        break;
                    }
                    case HIT_TYPE_LEGS:
                    {
                        send_set_hp(server, player_id, hit_player_id, 33, 1, 0, 5, 0, shot_pos);
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
                        send_set_hp(server, player_id, hit_player_id, 75, 1, 1, 5, 0, shot_pos);
                        break;
                    }
                    case HIT_TYPE_TORSO:
                    {
                        send_set_hp(server, player_id, hit_player_id, 29, 1, 0, 5, 0, shot_pos);
                        break;
                    }
                    case HIT_TYPE_ARMS:
                    {
                        send_set_hp(server, player_id, hit_player_id, 18, 1, 0, 5, 0, shot_pos);
                        break;
                    }
                    case HIT_TYPE_LEGS:
                    {
                        send_set_hp(server, player_id, hit_player_id, 18, 1, 0, 5, 0, shot_pos);
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
                        send_set_hp(server, player_id, hit_player_id, 37, 1, 1, 5, 0, shot_pos);
                        break;
                    }
                    case HIT_TYPE_TORSO:
                    {
                        send_set_hp(server, player_id, hit_player_id, 27, 1, 0, 5, 0, shot_pos);
                        break;
                    }
                    case HIT_TYPE_ARMS:
                    {
                        send_set_hp(server, player_id, hit_player_id, 16, 1, 0, 5, 0, shot_pos);
                        break;
                    }
                    case HIT_TYPE_LEGS:
                    {
                        send_set_hp(server, player_id, hit_player_id, 16, 1, 0, 5, 0, shot_pos);
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
            send_set_hp(server, player_id, hit_player_id, 80, 1, 2, 5, 0, shot_pos);
        }
    }
}
