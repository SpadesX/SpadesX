#include <Server/Packets/Packets.h>
#include <Server/ParseConvert.h>
#include <Server/Server.h>
#include <Server/Structs/PlayerStruct.h>
#include <Server/Structs/ServerStruct.h>
#include <Util/Enums.h>
#include <Util/Log.h>
#include <Util/Types.h>
#include <Util/Weapon.h>
#include <math.h>
#include <time.h>

uint8_t check_under_tent(server_t* server, uint8_t team)
{
    uint8_t count = 0;
    for (int x = server->protocol.gamemode.base[team].x - 1; x <= server->protocol.gamemode.base[team].x; x++) {
        for (int y = server->protocol.gamemode.base[team].y - 1; y <= server->protocol.gamemode.base[team].y; y++) {
            if (mapvxl_is_solid(&server->s_map.map, x, y, server->protocol.gamemode.base[team].z) == 0) {
                count++;
            }
        }
    }
    return count;
}

uint8_t check_under_intel(server_t* server, uint8_t team)
{
    uint8_t ret = 0;
    if (mapvxl_is_solid(&server->s_map.map,
                        server->protocol.gamemode.intel[team].x,
                        server->protocol.gamemode.intel[team].y,
                        server->protocol.gamemode.intel[team].z) == 0)
    {
        ret = 1;
    }
    return ret;
}

uint8_t check_in_tent(server_t* server, uint8_t team)
{
    uint8_t    ret      = 0;
    vector3f_t checkPos = server->protocol.gamemode.base[team];
    checkPos.z--;
    if (mapvxl_is_solid(&server->s_map.map, (int) checkPos.x, (int) checkPos.y, (int) checkPos.z))
    { // Implement check for solid blocks in XYZ range in libmapvxl
        ret = 1;
    } else if (mapvxl_is_solid(&server->s_map.map, (int) checkPos.x - 1, (int) checkPos.y, (int) checkPos.z)) {
        ret = 1;
    } else if (mapvxl_is_solid(&server->s_map.map, (int) checkPos.x, (int) checkPos.y - 1, (int) checkPos.z)) {
        ret = 1;
    } else if (mapvxl_is_solid(&server->s_map.map, (int) checkPos.x - 1, (int) checkPos.y - 1, (int) checkPos.z)) {
        ret = 1;
    }

    return ret;
}

uint8_t check_in_intel(server_t* server, uint8_t team)
{
    uint8_t    ret      = 0;
    vector3f_t checkPos = server->protocol.gamemode.intel[team];
    checkPos.z--;
    if (mapvxl_is_solid(&server->s_map.map, (int) checkPos.x, (int) checkPos.y, (int) checkPos.z)) {
        ret = 1;
    }
    return ret;
}

uint8_t check_player_in_tent(server_t* server, player_t* player)
{
    uint8_t ret = 0;
    if (player->alive == 0) {
        return ret;
    }
    vector3f_t playerPos = player->movement.position;
    vector3f_t tentPos   = server->protocol.gamemode.base[player->team];
    if (((int) playerPos.z + 3 == (int) tentPos.z ||
         ((player->crouching || playerPos.z < 0) && (int) playerPos.z + 2 == (int) tentPos.z)) &&
        ((int) playerPos.x >= (int) tentPos.x - 1 && (int) playerPos.x <= (int) tentPos.x) &&
        ((int) playerPos.y >= (int) tentPos.y - 1 && (int) playerPos.y <= (int) tentPos.y))
    {
        ret = 1;
    }
    return ret;
}

uint8_t check_item_on_intel(server_t* server, uint8_t team, vector3f_t itemPos)
{
    uint8_t    ret      = 0;
    vector3f_t intelPos = server->protocol.gamemode.intel[team];
    if ((int) itemPos.y == (int) intelPos.y && ((int) itemPos.z == (int) intelPos.z) &&
        (int) itemPos.x == (int) intelPos.x)
    {
        ret = 1;
    }
    return ret;
}

void move_intel_and_tent_down(server_t* server)
{
    while (check_under_intel(server, 0)) {
        vector3f_t newPos = {server->protocol.gamemode.intel[0].x,
                             server->protocol.gamemode.intel[0].y,
                             server->protocol.gamemode.intel[0].z + 1};
        send_move_object(server, 0, 0, newPos);
        server->protocol.gamemode.intel[0] = newPos;
    }
    while (check_under_intel(server, 1)) {
        vector3f_t newPos = {server->protocol.gamemode.intel[1].x,
                             server->protocol.gamemode.intel[1].y,
                             server->protocol.gamemode.intel[1].z + 1};
        send_move_object(server, 1, 1, newPos);
        server->protocol.gamemode.intel[1] = newPos;
    }
    while (check_under_tent(server, 0) == 4) {
        vector3f_t newPos = {server->protocol.gamemode.base[0].x,
                             server->protocol.gamemode.base[0].y,
                             server->protocol.gamemode.base[0].z + 1};
        send_move_object(server, 2, 0, newPos);
        server->protocol.gamemode.base[0] = newPos;
    }
    while (check_under_tent(server, 1) == 4) {
        vector3f_t newPos = {server->protocol.gamemode.base[1].x,
                             server->protocol.gamemode.base[1].y,
                             server->protocol.gamemode.base[1].z + 1};
        send_move_object(server, 3, 1, newPos);
        server->protocol.gamemode.base[1] = newPos;
    }
}

void moveIntelAndTentUp(server_t* server)
{
    if (check_in_tent(server, 0)) {
        vector3f_t newTentPos = server->protocol.gamemode.base[0];
        newTentPos.z -= 1;
        send_move_object(server, 0 + 2, 0, newTentPos);
        server->protocol.gamemode.base[0] = newTentPos;
    } else if (check_in_tent(server, 1)) {
        vector3f_t newTentPos = server->protocol.gamemode.base[1];
        newTentPos.z -= 1;
        send_move_object(server, 1 + 2, 1, newTentPos);
        server->protocol.gamemode.base[1] = newTentPos;
    } else if (check_in_intel(server, 1)) {
        vector3f_t newIntelPos = server->protocol.gamemode.intel[1];
        newIntelPos.z -= 1;
        send_move_object(server, 1, 1, newIntelPos);
        server->protocol.gamemode.intel[1] = newIntelPos;
    } else if (check_in_intel(server, 0)) {
        vector3f_t newIntelPos = server->protocol.gamemode.intel[0];
        newIntelPos.z -= 1;
        send_move_object(server, 0, 0, newIntelPos);
        server->protocol.gamemode.intel[0] = newIntelPos;
    }
}

uint8_t check_player_on_intel(server_t* server, player_t* player, uint8_t team)
{
    uint8_t ret = 0;
    if (player->alive == 0) {
        return ret;
    }
    vector3f_t player_pos     = player->movement.position;
    vector3f_t intel_pos      = server->protocol.gamemode.intel[team];
    vector3i_t player_pos_int = vec3f_to_vec3i(player_pos);
    vector3i_t intel_pos_int  = vec3f_to_vec3i(intel_pos);
    if (player_pos_int.y == intel_pos_int.y &&
        (player_pos_int.z + 3 == intel_pos_int.z ||
         (player_pos_int.z == server->s_map.map.size_z - 3 && player_pos_int.z + 2 == intel_pos_int.z)) &&
        player_pos_int.x == intel_pos_int.x)
    {
        ret = 1;
    }
    return ret;
}

uint8_t check_item_in_tent(server_t* server, uint8_t team, vector3f_t itemPos)
{
    uint8_t    ret     = 0;
    vector3f_t tentPos = server->protocol.gamemode.base[team];
    if (((int) itemPos.z == (int) tentPos.z) &&
        ((int) itemPos.x >= (int) tentPos.x - 1 && (int) itemPos.x <= (int) tentPos.x) &&
        ((int) itemPos.y >= (int) tentPos.y - 1 && (int) itemPos.y <= (int) tentPos.y))
    {
        ret = 1;
    }
    return ret;
}

vector3f_t set_intel_tent_spawn_point(server_t* server, uint8_t team)
{
    quad3d_t* spawn = server->protocol.spawns + team;

    float      dx = spawn->to.x - spawn->from.x;
    float      dy = spawn->to.y - spawn->from.y;
    vector3f_t position;
    position.x = spawn->from.x + dx * gen_rand(&server->rand);
    position.y = spawn->from.y + dy * gen_rand(&server->rand);
    position.z = mapvxl_find_top_block(&server->s_map.map, position.x, position.y);
    return position;
}

void handleTentAndIntel(server_t* server, player_t* player)
{
    uint8_t team;
    if (player->team == 0) {
        team = 1;
    } else {
        team = 0;
    }
    if (player->team != TEAM_SPECTATOR) {
        uint64_t timeNow = time(NULL);
        if (player->has_intel == 0) {

            if (check_player_on_intel(server, player, team) && (!server->protocol.gamemode.intel_held[team])) {
                send_intel_pickup(server, player);
                if (server->protocol.current_gamemode == GAME_MODE_BABEL) {
                    vector3f_t pos = {0, 0, 64};
                    send_move_object(server, player->team, player->team, pos);
                    server->protocol.gamemode.intel[player->team] = pos;
                }
            } else if (check_player_in_tent(server, player) &&
                       timeNow - player->timers.since_last_base_enter_restock >= 15)
            {
                send_restock(server, player);
                player->hp       = 100;
                player->grenades = 3;
                player->blocks   = 50;
                set_default_player_ammo_reserve(player);
                player->timers.since_last_base_enter_restock = time(NULL);
            }
        } else if (player->has_intel) {
            if (check_player_in_tent(server, player) && timeNow - player->timers.since_last_base_enter >= 5) {
                server->protocol.gamemode.score[player->team]++;
                uint8_t winning = 0;
                if (server->protocol.gamemode.score[player->team] >= server->protocol.gamemode.score_limit) {
                    winning = 1;
                }
                send_intel_capture(server, player, winning);
                player->hp       = 100;
                player->grenades = 3;
                player->blocks   = 50;
                set_default_player_ammo_reserve(player);
                send_restock(server, player);
                player->timers.since_last_base_enter = time(NULL);
                if (server->protocol.current_gamemode == GAME_MODE_BABEL) {
                    vector3f_t babelIntelPos = {255, 255, mapvxl_find_top_block(&server->s_map.map, 255, 255)};
                    server->protocol.gamemode.intel[0] = babelIntelPos;
                    server->protocol.gamemode.intel[1] = babelIntelPos;
                    send_move_object(server, 0, 0, server->protocol.gamemode.intel[0]);
                    send_move_object(server, 1, 1, server->protocol.gamemode.intel[1]);
                } else {
                    server->protocol.gamemode.intel[team] = set_intel_tent_spawn_point(server, team);
                    send_move_object(server, team, team, server->protocol.gamemode.intel[team]);
                }
                if (winning) {
                    player_t *connected_player, *tmp;
                    HASH_ITER(hh, server->players, connected_player, tmp)
                    {
                        if (connected_player->state != STATE_DISCONNECTED) {
                            connected_player->state = STATE_STARTING_MAP;
                        }
                    }
                    server_reset(server);
                }
            }
        }
    }
}
