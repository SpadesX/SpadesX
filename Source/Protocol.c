// Copyright DarkNeutrino 2021
#include "Protocol.h"

#include "../Extern/libmapvxl/libmapvxl.h"
#include "Packets.h"
#include "ParseConvert.h"
#include "Server.h"
#include "Structs.h"
#include "Util/DataStream.h"
#include "Util/Enums.h"
#include "Util/Log.h"
#include "Util/Physics.h"
#include "Util/Types.h"
#include "Util/Uthash.h"
#include "Util/Utlist.h"

#include <ctype.h>
#include <enet/enet.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

uint64_t get_nanos(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return (uint64_t) ts.tv_sec * 1000000000L + ts.tv_nsec;
}

/*uint8_t castRaysWithTolerance(Server *server, Vector3f pos, Vector3f posOrien, Vector3f othPlayerPos) {
    //TODO Actually do the function.
     Needs:
        Cross Product,
        3D Matrix,
        Vector 3DMatrix multiplication
        And possibly more im missing

}*/

static void copyBits(uint32_t* dst, uint32_t src, uint32_t endPos, uint32_t startPos)
{
    uint32_t numBitsToCopy = endPos - startPos + 1;
    uint32_t numBitsInInt  = sizeof(uint32_t) * 8;
    uint32_t zeroMask;
    uint32_t onesMask = ~((uint32_t) 0);

    onesMask = onesMask >> (numBitsInInt - numBitsToCopy);
    onesMask = onesMask << startPos;
    zeroMask = ~onesMask;
    *dst     = *dst & zeroMask;
    src      = src & onesMask;
    *dst     = *dst | src;
}

static void swapIPStruct(ip_t* ip)
{
    uint8_t tmp;
    tmp       = ip->ip[0];
    ip->ip[0] = ip->ip[3];
    ip->ip[3] = tmp;
    tmp       = ip->ip[1];
    ip->ip[1] = ip->ip[2];
    ip->ip[2] = tmp;
}

uint8_t octets_in_range(ip_t start, ip_t end, ip_t host)
{
    swapIPStruct(&start);
    swapIPStruct(&end);
    swapIPStruct(&host);
    if (host.ip32 >= start.ip32 && host.ip32 <= end.ip32) {
        return 1;
        swapIPStruct(&start);
        swapIPStruct(&end);
        swapIPStruct(&host);
    }
    swapIPStruct(&start);
    swapIPStruct(&end);
    swapIPStruct(&host);
    return 0;
}

uint8_t ip_in_range(ip_t host, ip_t banned, ip_t startOfRange, ip_t endOfRange)
{
    uint32_t max = UINT32_MAX;
    ip_t     startRange;
    ip_t     endRange;
    startRange.cidr = 24;
    endRange.cidr   = 24;
    startRange.ip32 = 0;
    endRange.ip32   = 0;
    startRange.ip32 = startOfRange.ip32;
    endRange.ip32   = endOfRange.ip32;
    if (banned.cidr > 0 && banned.cidr < 32) {
        uint32_t subMax = 0;
        copyBits(&subMax, max, 31 - banned.cidr, 0);
        swapIPStruct(&banned);
        uint32_t ourRange = 0;
        copyBits(&ourRange, banned.ip32, 32 - banned.cidr, 0);
        swapIPStruct(&banned);
        uint32_t times = ourRange / (subMax + 1);
        uint32_t start = times * (subMax + 1);
        ip_t     startIP;
        ip_t     endIP;
        startIP.ip32 = banned.ip32;
        endIP.ip32   = banned.ip32;
        startIP.cidr = 0;
        endIP.cidr   = 0;
        swapIPStruct(&startIP);
        swapIPStruct(&endIP);
        uint32_t end = ((times + 1) * subMax);
        end |= 1;
        copyBits(&startIP.ip32, start, 32 - banned.cidr, 0);
        copyBits(&endIP.ip32, end, 32 - banned.cidr, 0);
        swapIPStruct(&startIP);
        swapIPStruct(&endIP);
        if (octets_in_range(startIP, endIP, host)) {
            return 1;
        }
    } else if (octets_in_range(startRange, endRange, host)) {
        return 1;
    } else if (host.ip32 == banned.ip32) {
        return 1;
    }
    return 0;
}

static uint8_t grenadeGamemodeCheck(server_t* server, vector3f_t pos)
{
    if (gamemode_block_checks(server, pos.x + 1, pos.y + 1, pos.z + 1) &&
        gamemode_block_checks(server, pos.x + 1, pos.y + 1, pos.z - 1) &&
        gamemode_block_checks(server, pos.x + 1, pos.y + 1, pos.z) &&
        gamemode_block_checks(server, pos.x + 1, pos.y - 1, pos.z + 1) &&
        gamemode_block_checks(server, pos.x + 1, pos.y - 1, pos.z - 1) &&
        gamemode_block_checks(server, pos.x + 1, pos.y - 1, pos.z) &&
        gamemode_block_checks(server, pos.x - 1, pos.y + 1, pos.z + 1) &&
        gamemode_block_checks(server, pos.x - 1, pos.y + 1, pos.z - 1) &&
        gamemode_block_checks(server, pos.x - 1, pos.y + 1, pos.z) &&
        gamemode_block_checks(server, pos.x - 1, pos.y - 1, pos.z + 1) &&
        gamemode_block_checks(server, pos.x - 1, pos.y - 1, pos.z - 1) &&
        gamemode_block_checks(server, pos.x - 1, pos.y - 1, pos.z))
    {
        return 1;
    }
    return 0;
}

uint8_t player_has_permission(server_t* server, uint8_t player_id, uint8_t console, uint32_t permission)
{
    if (console) {
        return permission;
    } else {
        return (permission & server->player[player_id].permissions);
    }
}

uint8_t getPlayerUnstuck(server_t* server, uint8_t player_id)
{
    for (float z = server->player[player_id].movement.prev_legit_pos.z - 1;
         z <= server->player[player_id].movement.prev_legit_pos.z + 1;
         z++)
    {
        if (valid_pos_3f(server,
                         player_id,
                         server->player[player_id].movement.prev_legit_pos.x,
                         server->player[player_id].movement.prev_legit_pos.y,
                         z))
        {
            server->player[player_id].movement.prev_legit_pos.z = z;
            server->player[player_id].movement.position         = server->player[player_id].movement.prev_legit_pos;
            return 1;
        }
        for (float x = server->player[player_id].movement.prev_legit_pos.x - 1;
             x <= server->player[player_id].movement.prev_legit_pos.x + 1;
             x++)
        {
            for (float y = server->player[player_id].movement.prev_legit_pos.y - 1;
                 y <= server->player[player_id].movement.prev_legit_pos.y + 1;
                 y++)
            {
                if (valid_pos_3f(server, player_id, x, y, z)) {
                    server->player[player_id].movement.prev_legit_pos.x = x;
                    server->player[player_id].movement.prev_legit_pos.y = y;
                    server->player[player_id].movement.prev_legit_pos.z = z;
                    server->player[player_id].movement.position = server->player[player_id].movement.prev_legit_pos;
                    return 1;
                }
            }
        }
    }
    return 0;
}

uint8_t getGrenadeDamage(server_t* server, uint8_t damageID, grenade_t* grenade)
{
    double     diffX      = fabs(server->player[damageID].movement.position.x - grenade->position.x);
    double     diffY      = fabs(server->player[damageID].movement.position.y - grenade->position.y);
    double     diffZ      = fabs(server->player[damageID].movement.position.z - grenade->position.z);
    vector3f_t playerPos  = server->player[damageID].movement.position;
    vector3f_t grenadePos = grenade->position;
    if (diffX < 16 && diffY < 16 && diffZ < 16 &&
        physics_can_see(server, playerPos.x, playerPos.y, playerPos.z, grenadePos.x, grenadePos.y, grenadePos.z) &&
        grenade->position.z < 62)
    {
        double diff = ((diffX * diffX) + (diffY * diffY) + (diffZ * diffZ));
        if (diff == 0) {
            return 100;
        }
        return (int) fmin((4096.f / diff), 100);
    }
    return 0;
}

uint8_t gamemode_block_checks(server_t* server, int x, int y, int z)
{
    if (((((x >= 206 && x <= 306) && (y >= 240 && y <= 272) && (z == 2 || z == 0)) ||
          ((x >= 205 && x <= 307) && (y >= 239 && y <= 273) && (z == 1))) &&
         server->protocol.current_gamemode == GAME_MODE_BABEL))
    {
        return 0;
    }
    return 1;
}

void init_player(server_t*  server,
                 uint8_t    player_id,
                 uint8_t    reset,
                 uint8_t    disconnect,
                 vector3f_t empty,
                 vector3f_t forward,
                 vector3f_t strafe,
                 vector3f_t height)
{
    if (reset == 0) {
        server->player[player_id].state  = STATE_DISCONNECTED;
        server->player[player_id].queues = NULL;
    }
    server->player[player_id].ups                          = 60;
    server->player[player_id].timers.time_since_last_wu    = get_nanos();
    server->player[player_id].input                        = 0;
    server->player[player_id].movement.eye_pos             = empty;
    server->player[player_id].movement.forward_orientation = forward;
    server->player[player_id].movement.strafe_orientation  = strafe;
    server->player[player_id].movement.height_orientation  = height;
    server->player[player_id].movement.position            = empty;
    server->player[player_id].movement.velocity            = empty;
    if (reset == 0 && disconnect == 0) {
        permissions_t roleList[5] = {{"manager", &server->manager_passwd, 4},
                                     {"admin", &server->admin_passwd, 3},
                                     {"mod", &server->mod_passwd, 2},
                                     {"guard", &server->guard_passwd, 1},
                                     {"trusted", &server->trusted_passwd, 0}};
        for (unsigned long x = 0; x < sizeof(roleList) / sizeof(permissions_t); ++x) {
            server->player[player_id].role_list[x] = roleList[x];
        }
        server->player[player_id].grenade = NULL;
    }
    server->player[player_id].airborne                             = 0;
    server->player[player_id].wade                                 = 0;
    server->player[player_id].lastclimb                            = 0;
    server->player[player_id].move_backwards                       = 0;
    server->player[player_id].move_forward                         = 0;
    server->player[player_id].move_left                            = 0;
    server->player[player_id].move_right                           = 0;
    server->player[player_id].jumping                              = 0;
    server->player[player_id].crouching                            = 0;
    server->player[player_id].sneaking                             = 0;
    server->player[player_id].sprinting                            = 0;
    server->player[player_id].primary_fire                         = 0;
    server->player[player_id].secondary_fire                       = 0;
    server->player[player_id].can_build                            = 1;
    server->player[player_id].allow_killing                        = 1;
    server->player[player_id].allow_team_killing                   = 0;
    server->player[player_id].muted                                = 0;
    server->player[player_id].told_to_master                       = 0;
    server->player[player_id].timers.since_last_base_enter         = 0;
    server->player[player_id].timers.since_last_base_enter_restock = 0;
    server->player[player_id].timers.since_last_3block_dest        = 0;
    server->player[player_id].timers.since_last_block_dest         = 0;
    server->player[player_id].timers.since_last_block_plac         = 0;
    server->player[player_id].timers.since_last_grenade_thrown     = 0;
    server->player[player_id].timers.since_last_shot               = 0;
    server->player[player_id].timers.time_since_last_wu            = 0;
    server->player[player_id].timers.since_last_weapon_input       = 0;
    server->player[player_id].hp                                   = 100;
    server->player[player_id].blocks                               = 50;
    server->player[player_id].grenades                             = 3;
    server->player[player_id].has_intel                            = 0;
    server->player[player_id].reloading                            = 0;
    server->player[player_id].client                               = ' ';
    server->player[player_id].version_minor                        = 0;
    server->player[player_id].version_major                        = 0;
    server->player[player_id].version_revision                     = 0;
    server->player[player_id].periodic_delay_index                 = 0;
    server->player[player_id].current_periodic_message             = server->periodic_messages;
    server->player[player_id].welcome_sent                         = 0;
    if (reset == 0) {
        server->player[player_id].permissions = 0;
    } else if (reset == 1) {
        grenade_t* grenade;
        grenade_t* tmp;
        DL_FOREACH_SAFE(server->player[player_id].grenade, grenade, tmp)
        {
            DL_DELETE(server->player[player_id].grenade, grenade);
            free(grenade);
        }
    }
    server->player[player_id].is_invisible = 0;
    server->player[player_id].kills        = 0;
    server->player[player_id].deaths       = 0;
    memset(server->player[player_id].name, 0, 17);
    memset(server->player[player_id].os_info, 0, 255);
}

uint8_t isStaff(server_t* server, uint8_t player_id)
{
    if (player_has_permission(server, player_id, 0, 4294967294)) {
        return 1;
    }
    return 0;
}

void send_message_to_staff(server_t* server, const char* format, ...)
{
    va_list args;
    va_start(args, format);
    char fMessage[1024];
    vsnprintf(fMessage, 1024, format, args);
    va_end(args);

    send_server_notice(server, 32, 1, fMessage);

    for (uint8_t ID = 0; ID < server->protocol.max_players; ++ID) {
        if (is_past_join_screen(server, ID) && isStaff(server, ID)) {
            send_server_notice(server, ID, 0, fMessage);
        }
    }
}

uint8_t is_past_state_data(server_t* server, uint8_t player_id)
{
    player_t player = server->player[player_id];
    if (player.state == STATE_PICK_SCREEN || player.state == STATE_SPAWNING ||
        player.state == STATE_WAITING_FOR_RESPAWN || player.state == STATE_READY)
    {
        return 1;
    } else {
        return 0;
    }
}

uint8_t is_past_join_screen(server_t* server, uint8_t player_id)
{
    player_t player = server->player[player_id];
    if (player.state == STATE_SPAWNING || player.state == STATE_WAITING_FOR_RESPAWN || player.state == STATE_READY) {
        return 1;
    } else {
        return 0;
    }
}

uint8_t valid_pos_v3i(server_t* server, vector3i_t pos)
{
    if (pos.x < server->s_map.map.size_x && pos.x >= 0 && pos.y < server->s_map.map.size_y && pos.y >= 0 &&
        pos.z < server->s_map.map.size_z && pos.z >= 0)
    {
        return 1;
    } else {
        return 0;
    }
}

uint8_t valid_pos_v3f(server_t* server, vector3f_t pos)
{
    if (pos.x < server->s_map.map.size_x && pos.x >= 0 && pos.y < server->s_map.map.size_y && pos.y >= 0 &&
        pos.z < server->s_map.map.size_z && pos.z >= 0)
    {
        return 1;
    } else {
        return 0;
    }
}
uint8_t valid_pos_3i(server_t* server, int x, int y, int z)
{
    if (x < server->s_map.map.size_x && x >= 0 && y < server->s_map.map.size_y && y >= 0 &&
        z < server->s_map.map.size_z && z >= 0)
    {
        return 1;
    } else {
        return 0;
    }
}

uint8_t valid_pos_3f(server_t* server, uint8_t player_id, float X, float Y, float Z)
{
    int x = (int) X;
    int y = (int) Y;
    int z = 0;
    if (Z < 0.f) {
        z = (int) Z + 1;
    } else {
        z = (int) Z + 2;
    }

    if (x < server->s_map.map.size_x && x >= 0 && y < server->s_map.map.size_y && y >= 0 &&
        (z < server->s_map.map.size_z || (z == 64 && server->player[player_id].crouching)) &&
        (z >= 0 || ((z == -1) || (z == -2 && server->player[player_id].jumping))))
    {
        if ((!mapvxl_is_solid(&server->s_map.map, x, y, z) ||
             (z == 63 || z == -1 || (z == -2 && server->player[player_id].jumping) ||
              (z == 64 && server->player[player_id].crouching) || server->player[player_id].jumping) ||
             (mapvxl_is_solid(&server->s_map.map, x, y, z) && server->player[player_id].crouching)) &&
            (!mapvxl_is_solid(&server->s_map.map, x, y, z - 1) ||
             ((z <= 1 && z > -2) || (z == -2 && server->player[player_id].jumping)) ||
             (z - 1 == 63 && server->player[player_id].crouching)) &&
            (!mapvxl_is_solid(&server->s_map.map, x, y, z - 2) ||
             ((z <= 2 && z > -2) || (z == -2 && server->player[player_id].jumping))))
        /* Dont even think about this
        This is what happens when map doesnt account for full height of
        freaking player and I have to check for out of bounds checks on map...
        */
        {
            return 1;
        }
    }
    return 0;
}

vector3i_t* get_neighbours(vector3i_t pos)
{
    static vector3i_t neighArray[6];
    neighArray[0].x = pos.x;
    neighArray[0].y = pos.y;
    neighArray[0].z = pos.z - 1;

    neighArray[1].x = pos.x;
    neighArray[1].y = pos.y - 1;
    neighArray[1].z = pos.z;

    neighArray[2].x = pos.x;
    neighArray[2].y = pos.y + 1;
    neighArray[2].z = pos.z;

    neighArray[3].x = pos.x - 1;
    neighArray[3].y = pos.y;
    neighArray[3].z = pos.z;

    neighArray[4].x = pos.x + 1;
    neighArray[4].y = pos.y;
    neighArray[4].z = pos.z;

    neighArray[5].x = pos.x;
    neighArray[5].y = pos.y;
    neighArray[5].z = pos.z + 1;

    return neighArray;
}

vector3i_t* getGrenadeNeighbors(vector3i_t pos)
{
    static vector3i_t neighArray[54];
    // This is nasty but fast.
    uint8_t index = 0;
    //-z size
    neighArray[index].x = pos.x;
    neighArray[index].y = pos.y;
    neighArray[index].z = pos.z - 2;
    index++;
    neighArray[index].x = pos.x + 1;
    neighArray[index].y = pos.y;
    neighArray[index].z = pos.z - 2;
    index++;
    neighArray[index].x = pos.x - 1;
    neighArray[index].y = pos.y;
    neighArray[index].z = pos.z - 2;
    index++;
    neighArray[index].x = pos.x;
    neighArray[index].y = pos.y + 1;
    neighArray[index].z = pos.z - 2;
    index++;
    neighArray[index].x = pos.x;
    neighArray[index].y = pos.y - 1;
    neighArray[index].z = pos.z - 2;
    index++;
    neighArray[index].x = pos.x + 1;
    neighArray[index].y = pos.y + 1;
    neighArray[index].z = pos.z - 2;
    index++;
    neighArray[index].x = pos.x + 1;
    neighArray[index].y = pos.y - 1;
    neighArray[index].z = pos.z - 2;
    index++;
    neighArray[index].x = pos.x - 1;
    neighArray[index].y = pos.y + 1;
    neighArray[index].z = pos.z - 2;
    index++;
    neighArray[index].x = pos.x - 1;
    neighArray[index].y = pos.y - 1;
    neighArray[index].z = pos.z - 2;
    index++;
    //-y side
    neighArray[index].x = pos.x;
    neighArray[index].y = pos.y - 2;
    neighArray[index].z = pos.z;
    index++;
    neighArray[index].x = pos.x + 1;
    neighArray[index].y = pos.y - 2;
    neighArray[index].z = pos.z;
    index++;
    neighArray[index].x = pos.x - 1;
    neighArray[index].y = pos.y - 2;
    neighArray[index].z = pos.z;
    index++;
    neighArray[index].x = pos.x;
    neighArray[index].y = pos.y - 2;
    neighArray[index].z = pos.z + 1;
    index++;
    neighArray[index].x = pos.x;
    neighArray[index].y = pos.y - 2;
    neighArray[index].z = pos.z - 1;
    index++;
    neighArray[index].x = pos.x + 1;
    neighArray[index].y = pos.y - 2;
    neighArray[index].z = pos.z + 1;
    index++;
    neighArray[index].x = pos.x + 1;
    neighArray[index].y = pos.y - 2;
    neighArray[index].z = pos.z - 1;
    index++;
    neighArray[index].x = pos.x - 1;
    neighArray[index].y = pos.y - 2;
    neighArray[index].z = pos.z + 1;
    index++;
    neighArray[index].x = pos.x - 1;
    neighArray[index].y = pos.y - 2;
    neighArray[index].z = pos.z - 1;
    index++;
    //+y side
    neighArray[index].x = pos.x;
    neighArray[index].y = pos.y + 2;
    neighArray[index].z = pos.z;
    index++;
    neighArray[index].x = pos.x + 1;
    neighArray[index].y = pos.y + 2;
    neighArray[index].z = pos.z;
    index++;
    neighArray[index].x = pos.x - 1;
    neighArray[index].y = pos.y + 2;
    neighArray[index].z = pos.z;
    index++;
    neighArray[index].x = pos.x;
    neighArray[index].y = pos.y + 2;
    neighArray[index].z = pos.z + 1;
    index++;
    neighArray[index].x = pos.x;
    neighArray[index].y = pos.y + 2;
    neighArray[index].z = pos.z - 1;
    index++;
    neighArray[index].x = pos.x + 1;
    neighArray[index].y = pos.y + 2;
    neighArray[index].z = pos.z + 1;
    index++;
    neighArray[index].x = pos.x + 1;
    neighArray[index].y = pos.y + 2;
    neighArray[index].z = pos.z - 1;
    index++;
    neighArray[index].x = pos.x - 1;
    neighArray[index].y = pos.y + 2;
    neighArray[index].z = pos.z + 1;
    index++;
    neighArray[index].x = pos.x - 1;
    neighArray[index].y = pos.y + 2;
    neighArray[index].z = pos.z - 1;
    index++;
    //-x side
    neighArray[index].x = pos.x - 2;
    neighArray[index].y = pos.y;
    neighArray[index].z = pos.z;
    index++;
    neighArray[index].x = pos.x - 2;
    neighArray[index].y = pos.y + 1;
    neighArray[index].z = pos.z;
    index++;
    neighArray[index].x = pos.x - 2;
    neighArray[index].y = pos.y - 1;
    neighArray[index].z = pos.z;
    index++;
    neighArray[index].x = pos.x - 2;
    neighArray[index].y = pos.y;
    neighArray[index].z = pos.z + 1;
    index++;
    neighArray[index].x = pos.x - 2;
    neighArray[index].y = pos.y;
    neighArray[index].z = pos.z - 1;
    index++;
    neighArray[index].x = pos.x - 2;
    neighArray[index].y = pos.y + 1;
    neighArray[index].z = pos.z + 1;
    index++;
    neighArray[index].x = pos.x - 2;
    neighArray[index].y = pos.y + 1;
    neighArray[index].z = pos.z - 1;
    index++;
    neighArray[index].x = pos.x - 2;
    neighArray[index].y = pos.y - 1;
    neighArray[index].z = pos.z - 1;
    index++;
    neighArray[index].x = pos.x - 2;
    neighArray[index].y = pos.y - 1;
    neighArray[index].z = pos.z + 1;
    index++;
    //+x side
    neighArray[index].x = pos.x + 2;
    neighArray[index].y = pos.y;
    neighArray[index].z = pos.z;
    index++;
    neighArray[index].x = pos.x + 2;
    neighArray[index].y = pos.y + 1;
    neighArray[index].z = pos.z;
    index++;
    neighArray[index].x = pos.x + 2;
    neighArray[index].y = pos.y - 1;
    neighArray[index].z = pos.z;
    index++;
    neighArray[index].x = pos.x + 2;
    neighArray[index].y = pos.y;
    neighArray[index].z = pos.z + 1;
    index++;
    neighArray[index].x = pos.x + 2;
    neighArray[index].y = pos.y;
    neighArray[index].z = pos.z - 1;
    index++;
    neighArray[index].x = pos.x + 2;
    neighArray[index].y = pos.y + 1;
    neighArray[index].z = pos.z + 1;
    index++;
    neighArray[index].x = pos.x + 2;
    neighArray[index].y = pos.y + 1;
    neighArray[index].z = pos.z - 1;
    index++;
    neighArray[index].x = pos.x + 2;
    neighArray[index].y = pos.y - 1;
    neighArray[index].z = pos.z - 1;
    index++;
    neighArray[index].x = pos.x + 2;
    neighArray[index].y = pos.y - 1;
    neighArray[index].z = pos.z + 1;
    index++;
    //+z side
    neighArray[index].x = pos.x;
    neighArray[index].y = pos.y;
    neighArray[index].z = pos.z + 2;
    index++;
    neighArray[index].x = pos.x + 1;
    neighArray[index].y = pos.y;
    neighArray[index].z = pos.z + 2;
    index++;
    neighArray[index].x = pos.x - 1;
    neighArray[index].y = pos.y;
    neighArray[index].z = pos.z + 2;
    index++;
    neighArray[index].x = pos.x;
    neighArray[index].y = pos.y + 1;
    neighArray[index].z = pos.z + 2;
    index++;
    neighArray[index].x = pos.x;
    neighArray[index].y = pos.y - 1;
    neighArray[index].z = pos.z + 2;
    index++;
    neighArray[index].x = pos.x + 1;
    neighArray[index].y = pos.y + 1;
    neighArray[index].z = pos.z + 2;
    index++;
    neighArray[index].x = pos.x + 1;
    neighArray[index].y = pos.y - 1;
    neighArray[index].z = pos.z + 2;
    index++;
    neighArray[index].x = pos.x - 1;
    neighArray[index].y = pos.y + 1;
    neighArray[index].z = pos.z + 2;
    index++;
    neighArray[index].x = pos.x - 1;
    neighArray[index].y = pos.y - 1;
    neighArray[index].z = pos.z + 2;
    index++;

    return neighArray;
}

#define NODE_RESERVE_SIZE 250000
vector3i_t* nodes = NULL;
int         nodePos;
int         nodesSize;
map_node_t* visitedMap;

static inline void saveNode(int x, int y, int z)
{
    nodes[nodePos].x = x;
    nodes[nodePos].y = y;
    nodes[nodePos].z = z;
    nodePos++;
}

static inline const vector3i_t* returnCurrentNode(void)
{
    return &nodes[--nodePos];
}

static inline void addNode(server_t* server, int x, int y, int z)
{
    if ((x >= 0 && x < server->s_map.map.size_x) && (y >= 0 && y < server->s_map.map.size_y) &&
        (z >= 0 && z < server->s_map.map.size_z))
    {
        if (mapvxl_is_solid(&server->s_map.map, x, y, z)) {
            saveNode(x, y, z);
        }
    }
}

uint8_t check_node(server_t* server, vector3i_t position)
{
    if (valid_pos_v3i(server, position) && mapvxl_is_solid(&server->s_map.map, position.x, position.y, position.z) == 0)
    {
        return 1;
    }
    if (nodes == NULL) {
        nodes     = (vector3i_t*) malloc(sizeof(vector3i_t) * NODE_RESERVE_SIZE);
        nodesSize = NODE_RESERVE_SIZE;
    }
    nodePos    = 0;
    visitedMap = NULL;

    saveNode(position.x, position.y, position.z);

    while (nodePos > 0) {
        vector3i_t* old_nodes;
        if (nodePos >= nodesSize - 6) {
            nodesSize += NODE_RESERVE_SIZE;
            old_nodes = (vector3i_t*) realloc((void*) nodes, sizeof(vector3i_t) * nodesSize);
            if (!old_nodes) {
                free(nodes);
                return 0;
            }
            nodes = old_nodes;
        }
        const vector3i_t* currentNode = returnCurrentNode();
        position.z                    = currentNode->z;
        if (position.z >= server->s_map.map.size_z - 2) {
            map_node_t* delNode;
            map_node_t* tmpNode;
            if (visitedMap != NULL) {
                HASH_ITER(hh, visitedMap, delNode, tmpNode)
                {
                    HASH_DEL(visitedMap, delNode);
                    free(delNode);
                }
            }
            free(nodes);
            nodes = NULL;
            return 1;
        }
        position.x = currentNode->x;
        position.y = currentNode->y;

        // already visited?
        map_node_t* node;
        int         id = position.x * server->s_map.map.size_y * server->s_map.map.size_z +
                 position.y * server->s_map.map.size_z + position.z;
        HASH_FIND_INT(visitedMap, &id, node);
        if (node == NULL) {
            node          = (map_node_t*) malloc(sizeof *node);
            node->pos     = position;
            node->visited = 1;
            node->id      = id;
            HASH_ADD_INT(visitedMap, id, node);
            addNode(server, position.x, position.y, position.z - 1);
            addNode(server, position.x, position.y - 1, position.z);
            addNode(server, position.x, position.y + 1, position.z);
            addNode(server, position.x - 1, position.y, position.z);
            addNode(server, position.x + 1, position.y, position.z);
            addNode(server, position.x, position.y, position.z + 1);
        }
    }

    map_node_t* Node;
    map_node_t* tmpNode;
    HASH_ITER(hh, visitedMap, Node, tmpNode)
    {
        vector3i_t block = {Node->pos.x, Node->pos.y, Node->pos.z};
        if (valid_pos_v3i(server, block)) {
            mapvxl_set_air(&server->s_map.map, Node->pos.x, Node->pos.y, Node->pos.z);
        }
        HASH_DEL(visitedMap, Node);
        free(Node);
    }
    free(nodes);
    nodes = NULL;
    return 0;
}

void move_intel_and_tent_down(server_t* server)
{
    while (checkUnderIntel(server, 0)) {
        vector3f_t newPos = {server->protocol.gamemode.intel[0].x,
                             server->protocol.gamemode.intel[0].y,
                             server->protocol.gamemode.intel[0].z + 1};
        send_move_object(server, 0, 0, newPos);
        server->protocol.gamemode.intel[0] = newPos;
    }
    while (checkUnderIntel(server, 1)) {
        vector3f_t newPos = {server->protocol.gamemode.intel[1].x,
                             server->protocol.gamemode.intel[1].y,
                             server->protocol.gamemode.intel[1].z + 1};
        send_move_object(server, 1, 1, newPos);
        server->protocol.gamemode.intel[1] = newPos;
    }
    while (checkUnderTent(server, 0) == 4) {
        vector3f_t newPos = {server->protocol.gamemode.base[0].x,
                             server->protocol.gamemode.base[0].y,
                             server->protocol.gamemode.base[0].z + 1};
        send_move_object(server, 2, 0, newPos);
        server->protocol.gamemode.base[0] = newPos;
    }
    while (checkUnderTent(server, 1) == 4) {
        vector3f_t newPos = {server->protocol.gamemode.base[1].x,
                             server->protocol.gamemode.base[1].y,
                             server->protocol.gamemode.base[1].z + 1};
        send_move_object(server, 3, 1, newPos);
        server->protocol.gamemode.base[1] = newPos;
    }
}

void moveIntelAndTentUp(server_t* server)
{
    if (checkInTent(server, 0)) {
        vector3f_t newTentPos = server->protocol.gamemode.base[0];
        newTentPos.z -= 1;
        send_move_object(server, 0 + 2, 0, newTentPos);
        server->protocol.gamemode.base[0] = newTentPos;
    } else if (checkInTent(server, 1)) {
        vector3f_t newTentPos = server->protocol.gamemode.base[1];
        newTentPos.z -= 1;
        send_move_object(server, 1 + 2, 1, newTentPos);
        server->protocol.gamemode.base[1] = newTentPos;
    } else if (checkInIntel(server, 1)) {
        vector3f_t newIntelPos = server->protocol.gamemode.intel[1];
        newIntelPos.z -= 1;
        send_move_object(server, 1, 1, newIntelPos);
        server->protocol.gamemode.intel[1] = newIntelPos;
    } else if (checkInIntel(server, 0)) {
        vector3f_t newIntelPos = server->protocol.gamemode.intel[0];
        newIntelPos.z -= 1;
        send_move_object(server, 0, 0, newIntelPos);
        server->protocol.gamemode.intel[0] = newIntelPos;
    }
}

uint8_t checkUnderTent(server_t* server, uint8_t team)
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

uint8_t checkUnderIntel(server_t* server, uint8_t team)
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

uint8_t checkPlayerOnIntel(server_t* server, uint8_t player_id, uint8_t team)
{
    uint8_t ret = 0;
    if (server->player[player_id].alive == 0) {
        return ret;
    }
    vector3f_t playerPos = server->player[player_id].movement.position;
    vector3f_t intelPos  = server->protocol.gamemode.intel[team];
    if ((int) playerPos.y == (int) intelPos.y &&
        ((int) playerPos.z + 3 == (int) intelPos.z ||
         ((server->player[player_id].crouching || playerPos.z < 0) && (int) playerPos.z + 2 == (int) intelPos.z)) &&
        (int) playerPos.x == (int) intelPos.x)
    {
        ret = 1;
    }
    return ret;
}

uint8_t checkPlayerInTent(server_t* server, uint8_t player_id)
{
    uint8_t ret = 0;
    if (server->player[player_id].alive == 0) {
        return ret;
    }
    vector3f_t playerPos = server->player[player_id].movement.position;
    vector3f_t tentPos   = server->protocol.gamemode.base[server->player[player_id].team];
    if (((int) playerPos.z + 3 == (int) tentPos.z ||
         ((server->player[player_id].crouching || playerPos.z < 0) && (int) playerPos.z + 2 == (int) tentPos.z)) &&
        ((int) playerPos.x >= (int) tentPos.x - 1 && (int) playerPos.x <= (int) tentPos.x) &&
        ((int) playerPos.y >= (int) tentPos.y - 1 && (int) playerPos.y <= (int) tentPos.y))
    {
        ret = 1;
    }
    return ret;
}

uint8_t checkItemOnIntel(server_t* server, uint8_t team, vector3f_t itemPos)
{
    uint8_t    ret      = 0;
    vector3f_t intelPos = server->protocol.gamemode.intel[team];
    if ((int) itemPos.y == (int) intelPos.y && ((int) itemPos.z == (int) intelPos.z) &&
        (int) itemPos.x == (int) intelPos.x) {
        ret = 1;
    }
    return ret;
}

uint8_t checkItemInTent(server_t* server, uint8_t team, vector3f_t itemPos)
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

uint8_t checkInTent(server_t* server, uint8_t team)
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

uint8_t checkInIntel(server_t* server, uint8_t team)
{
    uint8_t    ret      = 0;
    vector3f_t checkPos = server->protocol.gamemode.intel[team];
    checkPos.z--;
    if (mapvxl_is_solid(&server->s_map.map, (int) checkPos.x, (int) checkPos.y, (int) checkPos.z)) {
        ret = 1;
    }
    return ret;
}

vector3f_t SetIntelTentSpawnPoint(server_t* server, uint8_t team)
{
    quad3d_t* spawn = server->protocol.spawns + team;

    float      dx = spawn->to.x - spawn->from.x;
    float      dy = spawn->to.y - spawn->from.y;
    vector3f_t position;
    position.x = spawn->from.x + dx * ((float) rand() / (float) RAND_MAX);
    position.y = spawn->from.y + dy * ((float) rand() / (float) RAND_MAX);
    position.z = mapvxl_find_top_block(&server->s_map.map, position.x, position.y);
    return position;
}

void handleTentAndIntel(server_t* server, uint8_t player_id)
{
    uint8_t team;
    if (server->player[player_id].team == 0) {
        team = 1;
    } else {
        team = 0;
    }
    if (server->player[player_id].team != TEAM_SPECTATOR) {
        uint64_t timeNow = time(NULL);
        if (server->player[player_id].has_intel == 0) {

            if (checkPlayerOnIntel(server, player_id, team) && (!server->protocol.gamemode.intel_held[team])) {
                send_intel_pickup(server, player_id);
                if (server->protocol.current_gamemode == GAME_MODE_BABEL) {
                    vector3f_t pos = {0, 0, 64};
                    send_move_object(server, server->player[player_id].team, server->player[player_id].team, pos);
                    server->protocol.gamemode.intel[server->player[player_id].team] = pos;
                }
            } else if (checkPlayerInTent(server, player_id) &&
                       timeNow - server->player[player_id].timers.since_last_base_enter_restock >= 15)
            {
                send_restock(server, player_id);
                server->player[player_id].hp       = 100;
                server->player[player_id].grenades = 3;
                server->player[player_id].blocks   = 50;
                switch (server->player[player_id].weapon) {
                    case 0:
                        server->player[player_id].weapon_reserve = 50;
                        break;
                    case 1:
                        server->player[player_id].weapon_reserve = 120;
                        break;
                    case 2:
                        server->player[player_id].weapon_reserve = 48;
                        break;
                }
                server->player[player_id].timers.since_last_base_enter_restock = time(NULL);
            }
        } else if (server->player[player_id].has_intel) {
            if (checkPlayerInTent(server, player_id) &&
                timeNow - server->player[player_id].timers.since_last_base_enter >= 5) {
                server->protocol.gamemode.score[server->player[player_id].team]++;
                uint8_t winning = 0;
                if (server->protocol.gamemode.score[server->player[player_id].team] >=
                    server->protocol.gamemode.score_limit) {
                    winning = 1;
                }
                send_intel_capture(server, player_id, winning);
                server->player[player_id].hp       = 100;
                server->player[player_id].grenades = 3;
                server->player[player_id].blocks   = 50;
                switch (server->player[player_id].weapon) {
                    case 0:
                        server->player[player_id].weapon_reserve = 50;
                        break;
                    case 1:
                        server->player[player_id].weapon_reserve = 120;
                        break;
                    case 2:
                        server->player[player_id].weapon_reserve = 48;
                        break;
                }
                send_restock(server, player_id);
                server->player[player_id].timers.since_last_base_enter = time(NULL);
                if (server->protocol.current_gamemode == GAME_MODE_BABEL) {
                    vector3f_t babelIntelPos = {255, 255, mapvxl_find_top_block(&server->s_map.map, 255, 255)};
                    server->protocol.gamemode.intel[0] = babelIntelPos;
                    server->protocol.gamemode.intel[1] = babelIntelPos;
                    send_move_object(server, 0, 0, server->protocol.gamemode.intel[0]);
                    send_move_object(server, 1, 1, server->protocol.gamemode.intel[1]);
                } else {
                    server->protocol.gamemode.intel[team] = SetIntelTentSpawnPoint(server, team);
                    send_move_object(server, team, team, server->protocol.gamemode.intel[team]);
                }
                if (winning) {
                    for (uint32_t i = 0; i < server->protocol.max_players; ++i) {
                        if (server->player[i].state != STATE_DISCONNECTED) {
                            server->player[i].state = STATE_STARTING_MAP;
                        }
                    }
                    server_reset(server);
                }
            }
        }
    }
}

void handleGrenade(server_t* server, uint8_t player_id)
{
    grenade_t* grenade;
    grenade_t* tmp;
    DL_FOREACH_SAFE(server->player[player_id].grenade, grenade, tmp)
    {
        if (grenade->sent) {
            physics_move_grenade(server, grenade);
            if ((get_nanos() - grenade->time_since_sent) / 1000000000.f >= grenade->fuse) {
                uint8_t allowToDestroy = 0;
                if (grenadeGamemodeCheck(server, grenade->position)) {
                    send_block_action(server,
                                      player_id,
                                      3,
                                      floor(grenade->position.x),
                                      floor(grenade->position.y),
                                      floor(grenade->position.z));
                    allowToDestroy = 1;
                }
                for (int y = 0; y < server->protocol.max_players; ++y) {
                    if (server->player[y].state == STATE_READY) {
                        uint8_t value = getGrenadeDamage(server, y, grenade);
                        if (value > 0) {
                            send_set_hp(server, player_id, y, value, 1, 3, 5, 1, grenade->position);
                        }
                    }
                }
                float x = grenade->position.x;
                float y = grenade->position.y;
                for (int z = grenade->position.z - 1; z <= grenade->position.z + 1; ++z) {
                    if (z < 62 &&
                        (x >= 0 && x <= server->s_map.map.size_x && x - 1 >= 0 && x - 1 <= server->s_map.map.size_x &&
                         x + 1 >= 0 && x + 1 <= server->s_map.map.size_x) &&
                        (y >= 0 && y <= server->s_map.map.size_y && y - 1 >= 0 && y - 1 <= server->s_map.map.size_y &&
                         y + 1 >= 0 && y + 1 <= server->s_map.map.size_y))
                    {
                        if (allowToDestroy) {
                            mapvxl_set_air(&server->s_map.map, x - 1, y - 1, z);
                            mapvxl_set_air(&server->s_map.map, x, y - 1, z);
                            mapvxl_set_air(&server->s_map.map, x + 1, y - 1, z);
                            mapvxl_set_air(&server->s_map.map, x - 1, y, z);
                            mapvxl_set_air(&server->s_map.map, x, y, z);
                            mapvxl_set_air(&server->s_map.map, x + 1, y, z);
                            mapvxl_set_air(&server->s_map.map, x - 1, y + 1, z);
                            mapvxl_set_air(&server->s_map.map, x, y + 1, z);
                            mapvxl_set_air(&server->s_map.map, x + 1, y + 1, z);
                        }
                        vector3i_t pos;
                        pos.x = grenade->position.x;
                        pos.y = grenade->position.y;
                        pos.z = grenade->position.z;

                        vector3i_t* neigh = getGrenadeNeighbors(pos);
                        for (int index = 0; index < 54; ++index) {
                            if (valid_pos_v3i(server, neigh[index])) {
                                check_node(server, neigh[index]);
                            }
                        }
                    }
                }
                grenade->sent = 0;
                DL_DELETE(server->player[player_id].grenade, grenade);
                free(grenade);
                move_intel_and_tent_down(server);
            }
        }
    }
}

void updateMovementAndGrenades(server_t* server)
{
    physics_set_globals((server->global_timers.update_time - server->global_timers.time_since_start) / 1000000000.f,
                        (server->global_timers.update_time - server->global_timers.last_update_time) / 1000000000.f);
    for (int player_id = 0; player_id < server->protocol.max_players; player_id++) {
        if (server->player[player_id].state == STATE_READY) {
            long falldamage = 0;
            falldamage      = physics_move_player(server, player_id);
            if (falldamage > 0) {
                vector3f_t zero = {0, 0, 0};
                send_set_hp(server, player_id, player_id, falldamage, 0, 4, 5, 0, zero);
            }
            if (server->protocol.current_gamemode != GAME_MODE_ARENA) {
                handleTentAndIntel(server, player_id);
            }
        }
        handleGrenade(server, player_id);
    }
}

void SetPlayerRespawnPoint(server_t* server, uint8_t player_id)
{
    if (server->player[player_id].team != TEAM_SPECTATOR) {
        quad3d_t* spawn = server->protocol.spawns + server->player[player_id].team;

        float dx = spawn->to.x - spawn->from.x;
        float dy = spawn->to.y - spawn->from.y;

        server->player[player_id].movement.position.x = spawn->from.x + dx * ((float) rand() / (float) RAND_MAX);
        server->player[player_id].movement.position.y = spawn->from.y + dy * ((float) rand() / (float) RAND_MAX);
        server->player[player_id].movement.position.z =
        mapvxl_find_top_block(&server->s_map.map,
                              server->player[player_id].movement.position.x,
                              server->player[player_id].movement.position.y) -
        2.36f;

        server->player[player_id].movement.forward_orientation.x = 0.f;
        server->player[player_id].movement.forward_orientation.y = 0.f;
        server->player[player_id].movement.forward_orientation.z = 0.f;
    }
}

void send_server_notice(server_t* server, uint8_t player_id, uint8_t console, const char* message, ...)
{
    va_list args;
    va_start(args, message);
    char fMessage[1024];
    vsnprintf(fMessage, 1024, message, args);
    va_end(args);

    if (console) {
        LOG_INFO("%s", fMessage);
        return;
    }

    uint32_t    fMessageSize = strlen(fMessage);
    uint32_t    packetSize   = 3 + fMessageSize;
    ENetPacket* packet       = enet_packet_create(NULL, packetSize, ENET_PACKET_FLAG_RELIABLE);
    stream_t    stream       = {packet->data, packet->dataLength, 0};
    stream_write_u8(&stream, PACKET_TYPE_CHAT_MESSAGE);
    stream_write_u8(&stream, player_id);
    stream_write_u8(&stream, 2);
    stream_write_array(&stream, fMessage, fMessageSize);
    if (enet_peer_send(server->player[player_id].peer, 0, packet) != 0) {
        enet_packet_destroy(packet);
    }
}

void broadcast_server_notice(server_t* server, uint8_t console, const char* message, ...)
{
    va_list args;
    va_start(args, message);
    char fMessage[1024];
    vsnprintf(fMessage, 1024, message, args);
    va_end(args);

    uint32_t    fMessageSize = strlen(fMessage);
    uint32_t    packetSize   = 3 + fMessageSize;
    ENetPacket* packet       = enet_packet_create(NULL, packetSize, ENET_PACKET_FLAG_RELIABLE);
    stream_t    stream       = {packet->data, packet->dataLength, 0};
    stream_write_u8(&stream, PACKET_TYPE_CHAT_MESSAGE);
    stream_write_u8(&stream, 33);
    stream_write_u8(&stream, 2);
    stream_write_array(&stream, fMessage, fMessageSize);
    if (console) {
        LOG_INFO("%s", fMessage);
    }
    uint8_t sent = 0;
    for (int player = 0; player < server->protocol.max_players; ++player) {
        if (is_past_join_screen(server, player)) {
            if (enet_peer_send(server->player[player].peer, 0, packet) == 0) {
                sent = 1;
            }
        }
    }
    if (sent == 0) {
        enet_packet_destroy(packet);
    }
}

uint8_t player_to_player_visibile(server_t* server, uint8_t player_id, uint8_t player_id2)
{
    float distance = 0;
    distance       = sqrt(
    fabs(pow((server->player[player_id].movement.position.x - server->player[player_id2].movement.position.x), 2)) +
    fabs(pow((server->player[player_id].movement.position.y - server->player[player_id2].movement.position.y), 2)));
    if (server->player[player_id].team == TEAM_SPECTATOR) {
        return 1;
    } else if (distance >= 132) {
        return 0;
    } else {
        return 1;
    }
}

uint32_t distance_in_3d(vector3f_t vector1, vector3f_t vector2)
{
    uint32_t distance = 0;
    distance          = sqrt(fabs(pow(vector1.x - vector2.x, 2)) + fabs(pow(vector1.y - vector2.y, 2)) +
                    fabs(pow(vector1.z - vector2.z, 2)));
    return distance;
}

uint32_t DistanceIn2D(vector3f_t vector1, vector3f_t vector2)
{
    uint32_t distance = 0;
    distance          = sqrt(fabs(pow(vector1.x - vector2.x, 2)) + fabs(pow(vector1.y - vector2.y, 2)));
    return distance;
}

uint8_t Collision3D(vector3f_t vector1, vector3f_t vector2, uint8_t distance)
{
    if (fabs(vector1.x - vector2.x) < distance && fabs(vector1.y - vector2.y) < distance &&
        fabs(vector1.x - vector2.x) < distance)
    {
        return 0;
    } else {
        return 1;
    }
}

uint8_t SendPacketExceptSender(server_t* server, ENetPacket* packet, uint8_t player_id)
{
    uint8_t sent = 0;
    for (uint8_t i = 0; i < 32; ++i) {
        if (player_id != i && is_past_state_data(server, i)) {
            if (enet_peer_send(server->player[i].peer, 0, packet) == 0) {
                sent = 1;
            }
        }
    }
    return sent;
}

uint8_t SendPacketExceptSenderDistCheck(server_t* server, ENetPacket* packet, uint8_t player_id)
{
    uint8_t sent = 0;
    for (uint8_t i = 0; i < 32; ++i) {
        if (player_id != i && is_past_state_data(server, i)) {
            if (player_to_player_visibile(server, player_id, i) || server->player[i].team == TEAM_SPECTATOR) {
                if (enet_peer_send(server->player[i].peer, 0, packet) == 0) {
                    sent = 1;
                }
            }
        }
    }
    return sent;
}

uint8_t SendPacketDistCheck(server_t* server, ENetPacket* packet, uint8_t player_id)
{
    uint8_t sent = 0;
    for (uint8_t i = 0; i < 32; ++i) {
        if (is_past_state_data(server, i)) {
            if (player_to_player_visibile(server, player_id, i) || server->player[i].team == TEAM_SPECTATOR) {
                if (enet_peer_send(server->player[i].peer, 0, packet) == 0) {
                    sent = 1;
                }
            }
        }
    }
    return sent;
}

uint8_t diff_is_older_then(uint64_t timeNow, uint64_t* timeBefore, uint64_t timeDiff)
{
    if (timeNow - *timeBefore >= timeDiff) {
        *timeBefore = timeNow;
        return 1;
    } else {
        return 0;
    }
}

uint8_t diff_is_older_then_dont_update(uint64_t timeNow, uint64_t timeBefore, uint64_t timeDiff)
{
    if (timeNow - timeBefore >= timeDiff) {
        return 1;
    } else {
        return 0;
    }
}
