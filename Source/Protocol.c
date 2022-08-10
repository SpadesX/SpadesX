// Copyright DarkNeutrino 2021
#include "Protocol.h"

#include "../Extern/libmapvxl/libmapvxl.h"
#include "Packets.h"
#include "ParseConvert.h"
#include "Server.h"
#include "Structs.h"
#include "Util/DataStream.h"
#include "Util/Enums.h"
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

uint64 getNanos(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return (uint64) ts.tv_sec * 1000000000L + ts.tv_nsec;
}

/*uint8 castRaysWithTolerance(Server *server, Vector3f pos, Vector3f posOrien, Vector3f othPlayerPos) {
    //TODO Actually do the function.
     Needs:
        Cross Product,
        3D Matrix,
        Vector 3DMatrix multiplication
        And possibly more im missing

}*/

static void copyBits(uint32* dest, uint32 src, uint32 endPos, uint32 startPos)
{
    uint32 numBitsToCopy = endPos - startPos + 1;
    uint32 numBitsInInt  = sizeof(uint32) * 8;
    uint32 zeroMask;
    uint32 onesMask = ~((uint32) 0);

    onesMask = onesMask >> (numBitsInInt - numBitsToCopy);
    onesMask = onesMask << startPos;
    zeroMask = ~onesMask;
    *dest    = *dest & zeroMask;
    src      = src & onesMask;
    *dest    = *dest | src;
}

static void swapIPStruct(IPStruct* ip)
{
    uint8 tmp;
    tmp             = ip->Union.ip[0];
    ip->Union.ip[0] = ip->Union.ip[3];
    ip->Union.ip[3] = tmp;
    tmp             = ip->Union.ip[1];
    ip->Union.ip[1] = ip->Union.ip[2];
    ip->Union.ip[2] = tmp;
}

uint8 octetsInRange(IPStruct start, IPStruct end, IPStruct host)
{
    swapIPStruct(&start);
    swapIPStruct(&end);
    swapIPStruct(&host);
    if (host.Union.ip32 >= start.Union.ip32 && host.Union.ip32 <= end.Union.ip32) {
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

uint8 IPInRange(IPStruct host, IPStruct banned, IPStruct startOfRange, IPStruct endOfRange)
{
    uint32   max = UINT32_MAX;
    IPStruct startRange;
    IPStruct endRange;
    startRange.CIDR = 24;
    endRange.CIDR = 24;
    startRange.Union.ip32 = 0;
    endRange.Union.ip32 = 0;
    startRange.Union.ip32 = startOfRange.Union.ip32;
    endRange.Union.ip32   = endOfRange.Union.ip32;
    if (banned.CIDR > 0 && banned.CIDR < 32) {
        uint32 subMax = 0;
        copyBits(&subMax, max, 31 - banned.CIDR, 0);
        swapIPStruct(&banned);
        uint32 ourRange = 0;
        copyBits(&ourRange, banned.Union.ip32, 32 - banned.CIDR, 0);
        swapIPStruct(&banned);
        uint32   times = ourRange / (subMax + 1);
        uint32   start = times * (subMax + 1);
        IPStruct startIP;
        IPStruct endIP;
        startIP.Union.ip32 = banned.Union.ip32;
        endIP.Union.ip32   = banned.Union.ip32;
        startIP.CIDR       = 0;
        endIP.CIDR         = 0;
        swapIPStruct(&startIP);
        swapIPStruct(&endIP);
        uint32 end = ((times + 1) * subMax);
        end |= 1;
        copyBits(&startIP.Union.ip32, start, 32 - banned.CIDR, 0);
        copyBits(&endIP.Union.ip32, end, 32 - banned.CIDR, 0);
        swapIPStruct(&startIP);
        swapIPStruct(&endIP);
        if (octetsInRange(startIP, endIP, host)) {
            return 1;
        }
    } else if (octetsInRange(startRange, endRange, host)) {
        return 1;
    } else if (host.Union.ip32 == banned.Union.ip32) {
        return 1;
    }
    return 0;
}

static uint8 grenadeGamemodeCheck(Server* server, Vector3f pos)
{
    if (gamemodeBlockChecks(server, pos.x + 1, pos.y + 1, pos.z + 1) &&
        gamemodeBlockChecks(server, pos.x + 1, pos.y + 1, pos.z - 1) &&
        gamemodeBlockChecks(server, pos.x + 1, pos.y + 1, pos.z) &&
        gamemodeBlockChecks(server, pos.x + 1, pos.y - 1, pos.z + 1) &&
        gamemodeBlockChecks(server, pos.x + 1, pos.y - 1, pos.z - 1) &&
        gamemodeBlockChecks(server, pos.x + 1, pos.y - 1, pos.z) &&
        gamemodeBlockChecks(server, pos.x - 1, pos.y + 1, pos.z + 1) &&
        gamemodeBlockChecks(server, pos.x - 1, pos.y + 1, pos.z - 1) &&
        gamemodeBlockChecks(server, pos.x - 1, pos.y + 1, pos.z) &&
        gamemodeBlockChecks(server, pos.x - 1, pos.y - 1, pos.z + 1) &&
        gamemodeBlockChecks(server, pos.x - 1, pos.y - 1, pos.z - 1) &&
        gamemodeBlockChecks(server, pos.x - 1, pos.y - 1, pos.z))
    {
        return 1;
    }
    return 0;
}

uint8 playerHasPermission(Server* server, uint8 playerID, uint8 console, uint32 permission)
{
    if (console) {
        return permission;
    } else {
        return (permission & server->player[playerID].permLevel);
    }
}

uint8 getPlayerUnstuck(Server* server, uint8 playerID)
{
    for (float z = server->player[playerID].movement.prevLegitPos.z - 1;
         z <= server->player[playerID].movement.prevLegitPos.z + 1;
         z++)
    {
        if (validPlayerPos(server,
                           playerID,
                           server->player[playerID].movement.prevLegitPos.x,
                           server->player[playerID].movement.prevLegitPos.y,
                           z))
        {
            server->player[playerID].movement.prevLegitPos.z = z;
            server->player[playerID].movement.position       = server->player[playerID].movement.prevLegitPos;
            return 1;
        }
        for (float x = server->player[playerID].movement.prevLegitPos.x - 1;
             x <= server->player[playerID].movement.prevLegitPos.x + 1;
             x++)
        {
            for (float y = server->player[playerID].movement.prevLegitPos.y - 1;
                 y <= server->player[playerID].movement.prevLegitPos.y + 1;
                 y++)
            {
                if (validPlayerPos(server, playerID, x, y, z)) {
                    server->player[playerID].movement.prevLegitPos.x = x;
                    server->player[playerID].movement.prevLegitPos.y = y;
                    server->player[playerID].movement.prevLegitPos.z = z;
                    server->player[playerID].movement.position       = server->player[playerID].movement.prevLegitPos;
                    return 1;
                }
            }
        }
    }
    return 0;
}

uint8 getGrenadeDamage(Server* server, uint8 damageID, Grenade* grenade)
{
    double   diffX      = fabs(server->player[damageID].movement.position.x - grenade->position.x);
    double   diffY      = fabs(server->player[damageID].movement.position.y - grenade->position.y);
    double   diffZ      = fabs(server->player[damageID].movement.position.z - grenade->position.z);
    Vector3f playerPos  = server->player[damageID].movement.position;
    Vector3f grenadePos = grenade->position;
    if (diffX < 16 && diffY < 16 && diffZ < 16 &&
        canSee(server, playerPos.x, playerPos.y, playerPos.z, grenadePos.x, grenadePos.y, grenadePos.z) &&
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

uint8 gamemodeBlockChecks(Server* server, int x, int y, int z)
{
    if (((((x >= 206 && x <= 306) && (y >= 240 && y <= 272) && (z == 2 || z == 0)) ||
          ((x >= 205 && x <= 307) && (y >= 239 && y <= 273) && (z == 1))) &&
         server->protocol.currentGameMode == GAME_MODE_BABEL))
    {
        return 0;
    }
    return 1;
}

void initPlayer(Server*  server,
                uint8    playerID,
                uint8    reset,
                uint8    disconnect,
                Vector3f empty,
                Vector3f forward,
                Vector3f strafe,
                Vector3f height)
{
    if (reset == 0) {
        server->player[playerID].state  = STATE_DISCONNECTED;
        server->player[playerID].queues = NULL;
    }
    server->player[playerID].ups                         = 60;
    server->player[playerID].timers.timeSinceLastWU      = getNanos();
    server->player[playerID].input                       = 0;
    server->player[playerID].movement.eyePos             = empty;
    server->player[playerID].movement.forwardOrientation = forward;
    server->player[playerID].movement.strafeOrientation  = strafe;
    server->player[playerID].movement.heightOrientation  = height;
    server->player[playerID].movement.position           = empty;
    server->player[playerID].movement.velocity           = empty;
    if (reset == 0 && disconnect == 0) {
        PermLevel roleList[5] = {{"manager", &server->managerPasswd, 4},
                                 {"admin", &server->adminPasswd, 3},
                                 {"mod", &server->modPasswd, 2},
                                 {"guard", &server->guardPasswd, 1},
                                 {"trusted", &server->trustedPasswd, 0}};
        for (unsigned long x = 0; x < sizeof(roleList) / sizeof(PermLevel); ++x) {
            server->player[playerID].roleList[x] = roleList[x];
        }
        server->player[playerID].grenade = NULL;
    }
    server->player[playerID].airborne                         = 0;
    server->player[playerID].wade                             = 0;
    server->player[playerID].lastclimb                        = 0;
    server->player[playerID].movBackwards                     = 0;
    server->player[playerID].movForward                       = 0;
    server->player[playerID].movLeft                          = 0;
    server->player[playerID].movRight                         = 0;
    server->player[playerID].jumping                          = 0;
    server->player[playerID].crouching                        = 0;
    server->player[playerID].sneaking                         = 0;
    server->player[playerID].sprinting                        = 0;
    server->player[playerID].primary_fire                     = 0;
    server->player[playerID].secondary_fire                   = 0;
    server->player[playerID].canBuild                         = 1;
    server->player[playerID].allowKilling                     = 1;
    server->player[playerID].allowTeamKilling                 = 0;
    server->player[playerID].muted                            = 0;
    server->player[playerID].toldToMaster                     = 0;
    server->player[playerID].timers.sinceLastBaseEnter        = 0;
    server->player[playerID].timers.sinceLastBaseEnterRestock = 0;
    server->player[playerID].timers.sinceLast3BlockDest       = 0;
    server->player[playerID].timers.sinceLastBlockDest        = 0;
    server->player[playerID].timers.sinceLastBlockPlac        = 0;
    server->player[playerID].timers.sinceLastGrenadeThrown    = 0;
    server->player[playerID].timers.sinceLastShot             = 0;
    server->player[playerID].timers.timeSinceLastWU           = 0;
    server->player[playerID].timers.sinceLastWeaponInput      = 0;
    server->player[playerID].HP                               = 100;
    server->player[playerID].blocks                           = 50;
    server->player[playerID].grenades                         = 3;
    server->player[playerID].hasIntel                         = 0;
    server->player[playerID].reloading                        = 0;
    server->player[playerID].toRefill                         = 0;
    server->player[playerID].client                           = ' ';
    server->player[playerID].version_minor                    = 0;
    server->player[playerID].version_major                    = 0;
    server->player[playerID].version_revision                 = 0;
    server->player[playerID].periodicDelayIndex               = 0;
    server->player[playerID].currentPeriodicMessage           = server->periodicMessages;
    server->player[playerID].welcomeSent                      = 0;
    if (reset == 0) {
        server->player[playerID].permLevel = 0;
    } else if (reset == 1) {
        Grenade* grenade;
        Grenade* tmp;
        DL_FOREACH_SAFE(server->player[playerID].grenade, grenade, tmp)
        {
            DL_DELETE(server->player[playerID].grenade, grenade);
            free(grenade);
        }
    }
    server->player[playerID].isInvisible = 0;
    server->player[playerID].kills       = 0;
    server->player[playerID].deaths      = 0;
    memset(server->player[playerID].name, 0, 17);
    memset(server->player[playerID].os_info, 0, 255);
}

uint8 isStaff(Server* server, uint8 playerID)
{
    if (playerHasPermission(server, playerID, 0, 4294967294)) {
        return 1;
    }
    return 0;
}

void sendMessageToStaff(Server* server, const char* format, ...)
{
    va_list args;
    va_start(args, format);
    char fMessage[1024];
    vsnprintf(fMessage, 1024, format, args);
    va_end(args);

    sendServerNotice(server, 32, 1, fMessage);

    for (uint8 ID = 0; ID < server->protocol.maxPlayers; ++ID) {
        if (isPastJoinScreen(server, ID) && isStaff(server, ID)) {
            sendServerNotice(server, ID, 0, fMessage);
        }
    }
}

uint8 isPastStateData(Server* server, uint8 playerID)
{
    Player player = server->player[playerID];
    if (player.state == STATE_PICK_SCREEN || player.state == STATE_SPAWNING ||
        player.state == STATE_WAITING_FOR_RESPAWN || player.state == STATE_READY)
    {
        return 1;
    } else {
        return 0;
    }
}

uint8 isPastJoinScreen(Server* server, uint8 playerID)
{
    Player player = server->player[playerID];
    if (player.state == STATE_SPAWNING || player.state == STATE_WAITING_FOR_RESPAWN || player.state == STATE_READY) {
        return 1;
    } else {
        return 0;
    }
}

uint8 vecValidPos(Server* server, Vector3i pos)
{
    if (pos.x < server->map.map.MAP_X_MAX && pos.x >= 0 && pos.y < server->map.map.MAP_Y_MAX && pos.y >= 0 &&
        pos.z < server->map.map.MAP_Z_MAX && pos.z >= 0)
    {
        return 1;
    } else {
        return 0;
    }
}

uint8 vecfValidPos(Server* server, Vector3f pos)
{
    if (pos.x < server->map.map.MAP_X_MAX && pos.x >= 0 && pos.y < server->map.map.MAP_Y_MAX && pos.y >= 0 &&
        pos.z < server->map.map.MAP_Z_MAX && pos.z >= 0)
    {
        return 1;
    } else {
        return 0;
    }
}
uint8 validPos(Server* server, int x, int y, int z)
{
    if (x < server->map.map.MAP_X_MAX && x >= 0 && y < server->map.map.MAP_Y_MAX && y >= 0 &&
        z < server->map.map.MAP_Z_MAX && z >= 0)
    {
        return 1;
    } else {
        return 0;
    }
}

uint8 validPlayerPos(Server* server, uint8 playerID, float X, float Y, float Z)
{
    int x = (int) X;
    int y = (int) Y;
    int z = 0;
    if (Z < 0.f) {
        z = (int) Z + 1;
    } else {
        z = (int) Z + 2;
    }

    if (x < server->map.map.MAP_X_MAX && x >= 0 && y < server->map.map.MAP_Y_MAX && y >= 0 &&
        (z < server->map.map.MAP_Z_MAX || (z == 64 && server->player[playerID].crouching)) &&
        (z >= 0 || ((z == -1) || (z == -2 && server->player[playerID].jumping))))
    {
        if ((!mapvxlIsSolid(&server->map.map, x, y, z) ||
             (z == 63 || z == -1 || (z == -2 && server->player[playerID].jumping) ||
              (z == 64 && server->player[playerID].crouching) || server->player[playerID].jumping) ||
             (mapvxlIsSolid(&server->map.map, x, y, z) && server->player[playerID].crouching)) &&
            (!mapvxlIsSolid(&server->map.map, x, y, z - 1) ||
             ((z <= 1 && z > -2) || (z == -2 && server->player[playerID].jumping)) ||
             (z - 1 == 63 && server->player[playerID].crouching)) &&
            (!mapvxlIsSolid(&server->map.map, x, y, z - 2) ||
             ((z <= 2 && z > -2) || (z == -2 && server->player[playerID].jumping))))
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

Vector3i* getNeighbors(Vector3i pos)
{
    static Vector3i neighArray[6];
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

Vector3i* getGrenadeNeighbors(Vector3i pos)
{
    static Vector3i neighArray[54];
    // This is nasty but fast.
    uint8 index = 0;
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
Vector3i* nodes = NULL;
int       nodePos;
int       nodesSize;
mapNode*  visitedMap;

static inline void saveNode(int x, int y, int z)
{
    nodes[nodePos].x = x;
    nodes[nodePos].y = y;
    nodes[nodePos].z = z;
    nodePos++;
}

static inline const Vector3i* returnCurrentNode()
{
    return &nodes[--nodePos];
}

static inline void addNode(Server* server, int x, int y, int z)
{
    if ((x >= 0 && x < server->map.map.MAP_X_MAX) && (y >= 0 && y < server->map.map.MAP_Y_MAX) &&
        (z >= 0 && z < server->map.map.MAP_Z_MAX))
    {
        if (mapvxlIsSolid(&server->map.map, x, y, z)) {
            saveNode(x, y, z);
        }
    }
}

uint8 checkNode(Server* server, Vector3i position)
{
    if (mapvxlIsSolid(&server->map.map, position.x, position.y, position.z) == 0) {
        return 1;
    }
    if (nodes == NULL) {
        nodes     = (Vector3i*) malloc(sizeof(Vector3i) * NODE_RESERVE_SIZE);
        nodesSize = NODE_RESERVE_SIZE;
    }
    nodePos    = 0;
    visitedMap = NULL;

    saveNode(position.x, position.y, position.z);

    while (nodePos > 0) {
        Vector3i* old_nodes;
        if (nodePos >= nodesSize - 6) {
            nodesSize += NODE_RESERVE_SIZE;
            old_nodes = (Vector3i*) realloc((void*) nodes, sizeof(Vector3i) * nodesSize);
            if (!old_nodes) {
                free(nodes);
                return 0;
            }
            nodes = old_nodes;
        }
        const Vector3i* currentNode = returnCurrentNode();
        position.z                  = currentNode->z;
        if (position.z >= server->map.map.MAP_Z_MAX - 2) {
            mapNode* delNode;
            mapNode* tmpNode;
            HASH_ITER(hh, visitedMap, delNode, tmpNode)
            {
                HASH_DEL(visitedMap, delNode);
                free(delNode);
            }
            free(nodes);
            nodes = NULL;
            return 1;
        }
        position.x = currentNode->x;
        position.y = currentNode->y;

        // already visited?
        mapNode* node;
        int      id = position.x * server->map.map.MAP_Y_MAX * server->map.map.MAP_Z_MAX +
                 position.y * server->map.map.MAP_Z_MAX + position.z;
        HASH_FIND_INT(visitedMap, &id, node);
        if (node == NULL) {
            node          = (mapNode*) malloc(sizeof *node);
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

    mapNode* Node;
    mapNode* tmpNode;
    HASH_ITER(hh, visitedMap, Node, tmpNode)
    {
        mapvxlSetAir(&server->map.map, Node->pos.x, Node->pos.y, Node->pos.z);
        HASH_DEL(visitedMap, Node);
        free(Node);
    }
    free(nodes);
    nodes = NULL;
    return 0;
}

void moveIntelAndTentDown(Server* server)
{
    while (checkUnderIntel(server, 0)) {
        Vector3f newPos = {server->protocol.gameMode.intel[0].x,
                           server->protocol.gameMode.intel[0].y,
                           server->protocol.gameMode.intel[0].z + 1};
        sendMoveObject(server, 0, 0, newPos);
        server->protocol.gameMode.intel[0] = newPos;
    }
    while (checkUnderIntel(server, 1)) {
        Vector3f newPos = {server->protocol.gameMode.intel[1].x,
                           server->protocol.gameMode.intel[1].y,
                           server->protocol.gameMode.intel[1].z + 1};
        sendMoveObject(server, 1, 1, newPos);
        server->protocol.gameMode.intel[1] = newPos;
    }
    while (checkUnderTent(server, 0) == 4) {
        Vector3f newPos = {server->protocol.gameMode.base[0].x,
                           server->protocol.gameMode.base[0].y,
                           server->protocol.gameMode.base[0].z + 1};
        sendMoveObject(server, 2, 0, newPos);
        server->protocol.gameMode.base[0] = newPos;
    }
    while (checkUnderTent(server, 1) == 4) {
        Vector3f newPos = {server->protocol.gameMode.base[1].x,
                           server->protocol.gameMode.base[1].y,
                           server->protocol.gameMode.base[1].z + 1};
        sendMoveObject(server, 3, 1, newPos);
        server->protocol.gameMode.base[1] = newPos;
    }
}

void moveIntelAndTentUp(Server* server)
{
    if (checkInTent(server, 0)) {
        Vector3f newTentPos = server->protocol.gameMode.base[0];
        newTentPos.z -= 1;
        sendMoveObject(server, 0 + 2, 0, newTentPos);
        server->protocol.gameMode.base[0] = newTentPos;
    } else if (checkInTent(server, 1)) {
        Vector3f newTentPos = server->protocol.gameMode.base[1];
        newTentPos.z -= 1;
        sendMoveObject(server, 1 + 2, 1, newTentPos);
        server->protocol.gameMode.base[1] = newTentPos;
    } else if (checkInIntel(server, 1)) {
        Vector3f newIntelPos = server->protocol.gameMode.intel[1];
        newIntelPos.z -= 1;
        sendMoveObject(server, 1, 1, newIntelPos);
        server->protocol.gameMode.intel[1] = newIntelPos;
    } else if (checkInIntel(server, 0)) {
        Vector3f newIntelPos = server->protocol.gameMode.intel[0];
        newIntelPos.z -= 1;
        sendMoveObject(server, 0, 0, newIntelPos);
        server->protocol.gameMode.intel[0] = newIntelPos;
    }
}

uint8 checkUnderTent(Server* server, uint8 team)
{
    uint8 count = 0;
    for (int x = server->protocol.gameMode.base[team].x - 1; x <= server->protocol.gameMode.base[team].x; x++) {
        for (int y = server->protocol.gameMode.base[team].y - 1; y <= server->protocol.gameMode.base[team].y; y++) {
            if (mapvxlIsSolid(&server->map.map, x, y, server->protocol.gameMode.base[team].z) == 0) {
                count++;
            }
        }
    }
    return count;
}

uint8 checkUnderIntel(Server* server, uint8 team)
{
    uint8 ret = 0;
    if (mapvxlIsSolid(&server->map.map,
                      server->protocol.gameMode.intel[team].x,
                      server->protocol.gameMode.intel[team].y,
                      server->protocol.gameMode.intel[team].z) == 0)
    {
        ret = 1;
    }
    return ret;
}

uint8 checkPlayerOnIntel(Server* server, uint8 playerID, uint8 team)
{
    uint8 ret = 0;
    if (server->player[playerID].alive == 0) {
        return ret;
    }
    Vector3f playerPos = server->player[playerID].movement.position;
    Vector3f intelPos  = server->protocol.gameMode.intel[team];
    if ((int) playerPos.y == (int) intelPos.y &&
        ((int) playerPos.z + 3 == (int) intelPos.z ||
         ((server->player[playerID].crouching || playerPos.z < 0) && (int) playerPos.z + 2 == (int) intelPos.z)) &&
        (int) playerPos.x == (int) intelPos.x)
    {
        ret = 1;
    }
    return ret;
}

uint8 checkPlayerInTent(Server* server, uint8 playerID)
{
    uint8 ret = 0;
    if (server->player[playerID].alive == 0) {
        return ret;
    }
    Vector3f playerPos = server->player[playerID].movement.position;
    Vector3f tentPos   = server->protocol.gameMode.base[server->player[playerID].team];
    if (((int) playerPos.z + 3 == (int) tentPos.z ||
         ((server->player[playerID].crouching || playerPos.z < 0) && (int) playerPos.z + 2 == (int) tentPos.z)) &&
        ((int) playerPos.x >= (int) tentPos.x - 1 && (int) playerPos.x <= (int) tentPos.x) &&
        ((int) playerPos.y >= (int) tentPos.y - 1 && (int) playerPos.y <= (int) tentPos.y))
    {
        ret = 1;
    }
    return ret;
}

uint8 checkItemOnIntel(Server* server, uint8 team, Vector3f itemPos)
{
    uint8    ret      = 0;
    Vector3f intelPos = server->protocol.gameMode.intel[team];
    if ((int) itemPos.y == (int) intelPos.y && ((int) itemPos.z == (int) intelPos.z) &&
        (int) itemPos.x == (int) intelPos.x) {
        ret = 1;
    }
    return ret;
}

uint8 checkItemInTent(Server* server, uint8 team, Vector3f itemPos)
{
    uint8    ret     = 0;
    Vector3f tentPos = server->protocol.gameMode.base[team];
    if (((int) itemPos.z == (int) tentPos.z) &&
        ((int) itemPos.x >= (int) tentPos.x - 1 && (int) itemPos.x <= (int) tentPos.x) &&
        ((int) itemPos.y >= (int) tentPos.y - 1 && (int) itemPos.y <= (int) tentPos.y))
    {
        ret = 1;
    }
    return ret;
}

uint8 checkInTent(Server* server, uint8 team)
{
    uint8    ret      = 0;
    Vector3f checkPos = server->protocol.gameMode.base[team];
    checkPos.z--;
    if (mapvxlIsSolid(&server->map.map, (int) checkPos.x, (int) checkPos.y, (int) checkPos.z))
    { // Implement check for solid blocks in XYZ range in libmapvxl
        ret = 1;
    } else if (mapvxlIsSolid(&server->map.map, (int) checkPos.x - 1, (int) checkPos.y, (int) checkPos.z)) {
        ret = 1;
    } else if (mapvxlIsSolid(&server->map.map, (int) checkPos.x, (int) checkPos.y - 1, (int) checkPos.z)) {
        ret = 1;
    } else if (mapvxlIsSolid(&server->map.map, (int) checkPos.x - 1, (int) checkPos.y - 1, (int) checkPos.z)) {
        ret = 1;
    }

    return ret;
}

uint8 checkInIntel(Server* server, uint8 team)
{
    uint8    ret      = 0;
    Vector3f checkPos = server->protocol.gameMode.intel[team];
    checkPos.z--;
    if (mapvxlIsSolid(&server->map.map, (int) checkPos.x, (int) checkPos.y, (int) checkPos.z)) {
        ret = 1;
    }
    return ret;
}

Vector3f SetIntelTentSpawnPoint(Server* server, uint8 team)
{
    Quad3D* spawn = server->protocol.spawns + team;

    float    dx = spawn->to.x - spawn->from.x;
    float    dy = spawn->to.y - spawn->from.y;
    Vector3f position;
    position.x = spawn->from.x + dx * ((float) rand() / (float) RAND_MAX);
    position.y = spawn->from.y + dy * ((float) rand() / (float) RAND_MAX);
    position.z = mapvxlFindTopBlock(&server->map.map, position.x, position.y);
    return position;
}

void handleTentAndIntel(Server* server, uint8 playerID)
{
    uint8 team;
    if (server->player[playerID].team == 0) {
        team = 1;
    } else {
        team = 0;
    }
    if (server->player[playerID].team != TEAM_SPECTATOR) {
        uint64 timeNow = time(NULL);
        if (server->player[playerID].hasIntel == 0) {

            if (checkPlayerOnIntel(server, playerID, team) && (!server->protocol.gameMode.intelHeld[team])) {
                sendIntelPickup(server, playerID);
                if (server->protocol.currentGameMode == GAME_MODE_BABEL) {
                    Vector3f pos = {0, 0, 64};
                    sendMoveObject(server, server->player[playerID].team, server->player[playerID].team, pos);
                    server->protocol.gameMode.intel[server->player[playerID].team] = pos;
                }
            } else if (checkPlayerInTent(server, playerID) &&
                       timeNow - server->player[playerID].timers.sinceLastBaseEnterRestock >= 15)
            {
                sendRestock(server, playerID);
                server->player[playerID].HP       = 100;
                server->player[playerID].grenades = 3;
                server->player[playerID].blocks   = 50;
                switch (server->player[playerID].weapon) {
                    case 0:
                        server->player[playerID].weaponReserve = 50;
                        break;
                    case 1:
                        server->player[playerID].weaponReserve = 120;
                        break;
                    case 2:
                        server->player[playerID].weaponReserve = 48;
                        break;
                }
                server->player[playerID].timers.sinceLastBaseEnterRestock = time(NULL);
            }
        } else if (server->player[playerID].hasIntel) {
            if (checkPlayerInTent(server, playerID) &&
                timeNow - server->player[playerID].timers.sinceLastBaseEnter >= 5) {
                server->protocol.gameMode.score[server->player[playerID].team]++;
                uint8 winning = 0;
                if (server->protocol.gameMode.score[server->player[playerID].team] >=
                    server->protocol.gameMode.scoreLimit) {
                    winning = 1;
                }
                sendIntelCapture(server, playerID, winning);
                server->player[playerID].HP       = 100;
                server->player[playerID].grenades = 3;
                server->player[playerID].blocks   = 50;
                switch (server->player[playerID].weapon) {
                    case 0:
                        server->player[playerID].weaponReserve = 50;
                        break;
                    case 1:
                        server->player[playerID].weaponReserve = 120;
                        break;
                    case 2:
                        server->player[playerID].weaponReserve = 48;
                        break;
                }
                sendRestock(server, playerID);
                server->player[playerID].timers.sinceLastBaseEnter = time(NULL);
                if (server->protocol.currentGameMode == GAME_MODE_BABEL) {
                    Vector3f babelIntelPos             = {255, 255, mapvxlFindTopBlock(&server->map.map, 255, 255)};
                    server->protocol.gameMode.intel[0] = babelIntelPos;
                    server->protocol.gameMode.intel[1] = babelIntelPos;
                    sendMoveObject(server, 0, 0, server->protocol.gameMode.intel[0]);
                    sendMoveObject(server, 1, 1, server->protocol.gameMode.intel[1]);
                } else {
                    server->protocol.gameMode.intel[team] = SetIntelTentSpawnPoint(server, team);
                    sendMoveObject(server, team, team, server->protocol.gameMode.intel[team]);
                }
                if (winning) {
                    for (uint32 i = 0; i < server->protocol.maxPlayers; ++i) {
                        if (server->player[i].state != STATE_DISCONNECTED) {
                            server->player[i].state = STATE_STARTING_MAP;
                        }
                    }
                    ServerReset(server);
                }
            }
        }
    }
}

void handleGrenade(Server* server, uint8 playerID)
{
    Grenade* grenade;
    Grenade* tmp;
    DL_FOREACH_SAFE(server->player[playerID].grenade, grenade, tmp)
    {
        if (grenade->sent) {
            moveGrenade(server, grenade);
            if ((getNanos() - grenade->timeSinceSent) / 1000000000.f >= grenade->fuse) {
                uint8 allowToDestroy = 0;
                if (grenadeGamemodeCheck(server, grenade->position)) {
                    sendBlockAction(server,
                                    playerID,
                                    3,
                                    floor(grenade->position.x),
                                    floor(grenade->position.y),
                                    floor(grenade->position.z));
                    allowToDestroy = 1;
                }
                for (int y = 0; y < server->protocol.maxPlayers; ++y) {
                    if (server->player[y].state == STATE_READY) {
                        uint8 value = getGrenadeDamage(server, y, grenade);
                        if (value > 0) {
                            sendSetHP(server, playerID, y, value, 1, 3, 5, 1, grenade->position);
                        }
                    }
                }
                float x = grenade->position.x;
                float y = grenade->position.y;
                for (int z = grenade->position.z - 1; z <= grenade->position.z + 1; ++z) {
                    if (z < 62 &&
                        (x >= 0 && x <= server->map.map.MAP_X_MAX && x - 1 >= 0 && x - 1 <= server->map.map.MAP_X_MAX &&
                         x + 1 >= 0 && x + 1 <= server->map.map.MAP_X_MAX) &&
                        (y >= 0 && y <= server->map.map.MAP_Y_MAX && y - 1 >= 0 && y - 1 <= server->map.map.MAP_Y_MAX &&
                         y + 1 >= 0 && y + 1 <= server->map.map.MAP_Y_MAX))
                    {
                        if (allowToDestroy) {
                            mapvxlSetAir(&server->map.map, x - 1, y - 1, z);
                            mapvxlSetAir(&server->map.map, x, y - 1, z);
                            mapvxlSetAir(&server->map.map, x + 1, y - 1, z);
                            mapvxlSetAir(&server->map.map, x - 1, y, z);
                            mapvxlSetAir(&server->map.map, x, y, z);
                            mapvxlSetAir(&server->map.map, x + 1, y, z);
                            mapvxlSetAir(&server->map.map, x - 1, y + 1, z);
                            mapvxlSetAir(&server->map.map, x, y + 1, z);
                            mapvxlSetAir(&server->map.map, x + 1, y + 1, z);
                        }
                        Vector3i pos;
                        pos.x = grenade->position.x;
                        pos.y = grenade->position.y;
                        pos.z = grenade->position.z;

                        Vector3i* neigh = getGrenadeNeighbors(pos);
                        for (int index = 0; index < 54; ++index) {
                            if (vecValidPos(server, neigh[index])) {
                                checkNode(server, neigh[index]);
                            }
                        }
                    }
                }
                grenade->sent = 0;
                DL_DELETE(server->player[playerID].grenade, grenade);
                free(grenade);
                moveIntelAndTentDown(server);
            }
        }
    }
}

void updateMovementAndGrenades(Server* server)
{
    setPhysicsGlobals((server->globalTimers.updateTime - server->globalTimers.timeSinceStart) / 1000000000.f,
                      (server->globalTimers.updateTime - server->globalTimers.lastUpdateTime) / 1000000000.f);
    for (int playerID = 0; playerID < server->protocol.maxPlayers; playerID++) {
        if (server->player[playerID].state == STATE_READY) {
            long falldamage = 0;
            falldamage      = movePlayer(server, playerID);
            if (falldamage > 0) {
                Vector3f zero = {0, 0, 0};
                sendSetHP(server, playerID, playerID, falldamage, 0, 4, 5, 0, zero);
            }
            if (server->protocol.currentGameMode != GAME_MODE_ARENA) {
                handleTentAndIntel(server, playerID);
            }
        }
        handleGrenade(server, playerID);
    }
}

void SetPlayerRespawnPoint(Server* server, uint8 playerID)
{
    if (server->player[playerID].team != TEAM_SPECTATOR) {
        Quad3D* spawn = server->protocol.spawns + server->player[playerID].team;

        float dx = spawn->to.x - spawn->from.x;
        float dy = spawn->to.y - spawn->from.y;

        server->player[playerID].movement.position.x = spawn->from.x + dx * ((float) rand() / (float) RAND_MAX);
        server->player[playerID].movement.position.y = spawn->from.y + dy * ((float) rand() / (float) RAND_MAX);
        server->player[playerID].movement.position.z =
        mapvxlFindTopBlock(
        &server->map.map, server->player[playerID].movement.position.x, server->player[playerID].movement.position.y) -
        2.36f;

        server->player[playerID].movement.forwardOrientation.x = 0.f;
        server->player[playerID].movement.forwardOrientation.y = 0.f;
        server->player[playerID].movement.forwardOrientation.z = 0.f;
    }
}

void sendServerNotice(Server* server, uint8 playerID, uint8 console, const char* message, ...)
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

    uint32      fMessageSize = strlen(fMessage);
    uint32      packetSize   = 3 + fMessageSize;
    ENetPacket* packet       = enet_packet_create(NULL, packetSize, ENET_PACKET_FLAG_RELIABLE);
    DataStream  stream       = {packet->data, packet->dataLength, 0};
    WriteByte(&stream, PACKET_TYPE_CHAT_MESSAGE);
    WriteByte(&stream, playerID);
    WriteByte(&stream, 2);
    WriteArray(&stream, fMessage, fMessageSize);
    if (enet_peer_send(server->player[playerID].peer, 0, packet) != 0) {
        enet_packet_destroy(packet);
    }
}

void broadcastServerNotice(Server* server, uint8 console, const char* message, ...)
{
    va_list args;
    va_start(args, message);
    char fMessage[1024];
    vsnprintf(fMessage, 1024, message, args);
    va_end(args);

    uint32      fMessageSize = strlen(fMessage);
    uint32      packetSize   = 3 + fMessageSize;
    ENetPacket* packet       = enet_packet_create(NULL, packetSize, ENET_PACKET_FLAG_RELIABLE);
    DataStream  stream       = {packet->data, packet->dataLength, 0};
    WriteByte(&stream, PACKET_TYPE_CHAT_MESSAGE);
    WriteByte(&stream, 33);
    WriteByte(&stream, 2);
    WriteArray(&stream, fMessage, fMessageSize);
    if (console) {
        LOG_INFO("%s", fMessage);
    }
    uint8 sent = 0;
    for (int player = 0; player < server->protocol.maxPlayers; ++player) {
        if (isPastJoinScreen(server, player)) {
            if (enet_peer_send(server->player[player].peer, 0, packet) == 0) {
                sent = 1;
            }
        }
    }
    if (sent == 0) {
        enet_packet_destroy(packet);
    }
}

uint8 playerToPlayerVisible(Server* server, uint8 playerID, uint8 playerID2)
{
    float distance = 0;
    distance =
    sqrt(fabs(pow((server->player[playerID].movement.position.x - server->player[playerID2].movement.position.x), 2)) +
         fabs(pow((server->player[playerID].movement.position.y - server->player[playerID2].movement.position.y), 2)));
    if (server->player[playerID].team == TEAM_SPECTATOR) {
        return 1;
    } else if (distance >= 132) {
        return 0;
    } else {
        return 1;
    }
}

uint32 DistanceIn3D(Vector3f vector1, Vector3f vector2)
{
    uint32 distance = 0;
    distance        = sqrt(fabs(pow(vector1.x - vector2.x, 2)) + fabs(pow(vector1.y - vector2.y, 2)) +
                    fabs(pow(vector1.z - vector2.z, 2)));
    return distance;
}

uint32 DistanceIn2D(Vector3f vector1, Vector3f vector2)
{
    uint32 distance = 0;
    distance        = sqrt(fabs(pow(vector1.x - vector2.x, 2)) + fabs(pow(vector1.y - vector2.y, 2)));
    return distance;
}

uint8 Collision3D(Vector3f vector1, Vector3f vector2, uint8 distance)
{
    if (fabs(vector1.x - vector2.x) < distance && fabs(vector1.y - vector2.y) < distance &&
        fabs(vector1.x - vector2.x) < distance)
    {
        return 0;
    } else {
        return 1;
    }
}

uint8 SendPacketExceptSender(Server* server, ENetPacket* packet, uint8 playerID)
{
    uint8 sent = 0;
    for (uint8 i = 0; i < 32; ++i) {
        if (playerID != i && isPastStateData(server, i)) {
            if (enet_peer_send(server->player[i].peer, 0, packet) == 0) {
                sent = 1;
            }
        }
    }
    return sent;
}

uint8 SendPacketExceptSenderDistCheck(Server* server, ENetPacket* packet, uint8 playerID)
{
    uint8 sent = 0;
    for (uint8 i = 0; i < 32; ++i) {
        if (playerID != i && isPastStateData(server, i)) {
            if (playerToPlayerVisible(server, playerID, i) || server->player[i].team == TEAM_SPECTATOR) {
                if (enet_peer_send(server->player[i].peer, 0, packet) == 0) {
                    sent = 1;
                }
            }
        }
    }
    return sent;
}

uint8 SendPacketDistCheck(Server* server, ENetPacket* packet, uint8 playerID)
{
    uint8 sent = 0;
    for (uint8 i = 0; i < 32; ++i) {
        if (isPastStateData(server, i)) {
            if (playerToPlayerVisible(server, playerID, i) || server->player[i].team == TEAM_SPECTATOR) {
                if (enet_peer_send(server->player[i].peer, 0, packet) == 0) {
                    sent = 1;
                }
            }
        }
    }
    return sent;
}

uint8 diffIsOlderThen(uint64 timeNow, uint64* timeBefore, uint64 timeDiff)
{
    if (timeNow - *timeBefore >= timeDiff) {
        *timeBefore = timeNow;
        return 1;
    } else {
        return 0;
    }
}

uint8 diffIsOlderThenDontUpdate(uint64 timeNow, uint64 timeBefore, uint64 timeDiff)
{
    if (timeNow - timeBefore >= timeDiff) {
        return 1;
    } else {
        return 0;
    }
}
