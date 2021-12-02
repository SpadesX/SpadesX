// Copyright DarkNeutrino 2021
#include "Enums.h"
#include "Packets.h"
#include "Physics.h"
#include "Server.h"
#include "Structs.h"

#include <DataStream.h>
#include <Types.h>
#include <ctype.h>
#include <enet/enet.h>
#include <libmapvxl/libmapvxl.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static unsigned long long get_nanos(void)
{
    struct timespec ts;
    timespec_get(&ts, TIME_UTC);
    return (unsigned long long) ts.tv_sec * 1000000000L + ts.tv_nsec;
}

/*uint8 castRaysWithTolerance(Server *server, Vector3f pos, Vector3f posOrien, Vector3f othPlayerPos) {
    //TODO Actually do the function.
     Needs:
        Cross Product,
        3D Matrix,
        Vector 3DMatrix multiplication
        And possibly more im missing

}*/

uint8 isStaff(Server* server, uint8 playerID)
{
    if (server->player[playerID].isManager || server->player[playerID].isAdmin || server->player[playerID].isMod ||
        server->player[playerID].isGuard)
    {
        return 1;
    }
    return 0;
}

void sendMessageToStaff(Server* server, char* message)
{
    for (uint8 ID = 0; ID < server->protocol.maxPlayers; ++ID) {
        if (isPastJoinScreen(server, ID) && (server->player[ID].isManager || server->player[ID].isAdmin ||
                                             server->player[ID].isMod || server->player[ID].isGuard))
        {
            sendServerNotice(server, ID, message);
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

uint8 vecValidPos(Vector3i pos)
{
    if (pos.x < MAP_MAX_X && pos.x >= 0 && pos.y < MAP_MAX_Y && pos.y >= 0 && pos.z < MAP_MAX_Z && pos.z >= 0) {
        return 1;
    } else {
        return 0;
    }
}

uint8 vecfValidPos(Vector3f pos)
{
    if (pos.x < MAP_MAX_X && pos.x >= 0 && pos.y < MAP_MAX_Y && pos.y >= 0 && pos.z < MAP_MAX_Z && pos.z >= 0) {
        return 1;
    } else {
        return 0;
    }
}
uint8 validPos(int x, int y, int z)
{
    if (x < MAP_MAX_X && x >= 0 && y < MAP_MAX_Y && y >= 0 && z < MAP_MAX_Z && z >= 0) {
        return 1;
    } else {
        return 0;
    }
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
Vector3i* visitedNodes = NULL;
int       visitedSize;
int       visitedPos;

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
    if ((x >= 0 && x < MAP_MAX_X) && (y >= 0 && y < MAP_MAX_Y) && (z >= 0 && z < MAP_MAX_Z)) {
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
    nodePos = 0;

    if (visitedNodes == NULL) {
        visitedNodes = (Vector3i*) malloc(sizeof(Vector3i) * NODE_RESERVE_SIZE);
        visitedSize  = NODE_RESERVE_SIZE;
    }
    visitedPos = 0;

    saveNode(position.x, position.y, position.z);

    while (nodePos > 0) {
        if (nodePos >= nodesSize - 6) {
            nodesSize += NODE_RESERVE_SIZE;
            nodes = (Vector3i*) realloc((void*) nodes, sizeof(Vector3i) * nodesSize);
        }
        if (visitedPos >= visitedSize - 6) {
            visitedSize += NODE_RESERVE_SIZE;
            visitedNodes = (Vector3i*) realloc((void*) visitedNodes, sizeof(Vector3i) * visitedSize);
        }
        const Vector3i* currentNode = returnCurrentNode();
        position.z                  = currentNode->z;
        if (position.z >= 62) {
            free(visitedNodes);
            visitedNodes = NULL;
            free(nodes);
            nodes = NULL;
            return 1;
        }
        position.x = currentNode->x;
        position.y = currentNode->y;

        // already visited?
        uint8 nodeVisited = 0;
        for (int i = 0; i < visitedPos; ++i) {
            if (visitedNodes[i].x == position.x && visitedNodes[i].y == position.y && visitedNodes[i].z == position.z) {
                nodeVisited = 1;
            }
        }
        if (nodeVisited == 0) {
            visitedNodes[visitedPos] = position;
            visitedPos++;
            addNode(server, position.x, position.y, position.z - 1);
            addNode(server, position.x, position.y - 1, position.z);
            addNode(server, position.x, position.y + 1, position.z);
            addNode(server, position.x - 1, position.y, position.z);
            addNode(server, position.x + 1, position.y, position.z);
            addNode(server, position.x, position.y, position.z + 1);
        }
    }

    for (int i = 0; i < visitedPos; ++i) {
        mapvxlSetAir(&server->map.map, visitedNodes[i].x, visitedNodes[i].y, visitedNodes[i].z);
    }

    free(visitedNodes);
    visitedNodes = NULL;
    free(nodes);
    nodes = NULL;
    return 0;
}

void moveIntelAndTentDown(Server* server)
{
    while (checkUnderIntel(server, 0)) {
        Vector3f newPos = {
        server->protocol.ctf.intel[0].x, server->protocol.ctf.intel[0].y, server->protocol.ctf.intel[0].z + 1};
        SendMoveObject(server, 0, 0, newPos);
        server->protocol.ctf.intel[0] = newPos;
    }
    while (checkUnderIntel(server, 1)) {
        Vector3f newPos = {
        server->protocol.ctf.intel[1].x, server->protocol.ctf.intel[1].y, server->protocol.ctf.intel[1].z + 1};
        SendMoveObject(server, 1, 1, newPos);
        server->protocol.ctf.intel[1] = newPos;
    }
    while (checkUnderTent(server, 0) == 4) {
        Vector3f newPos = {
        server->protocol.ctf.base[0].x, server->protocol.ctf.base[0].y, server->protocol.ctf.base[0].z + 1};
        SendMoveObject(server, 2, 0, newPos);
        server->protocol.ctf.base[0] = newPos;
    }
    while (checkUnderTent(server, 1) == 4) {
        Vector3f newPos = {
        server->protocol.ctf.base[1].x, server->protocol.ctf.base[1].y, server->protocol.ctf.base[1].z + 1};
        SendMoveObject(server, 3, 1, newPos);
        server->protocol.ctf.base[1] = newPos;
    }
}

void moveIntelAndTentUp(Server* server)
{
    if (checkInTent(server, 0)) {
        Vector3f newTentPos = server->protocol.ctf.base[0];
        newTentPos.z -= 1;
        SendMoveObject(server, 0 + 2, 0, newTentPos);
        server->protocol.ctf.base[0] = newTentPos;
    } else if (checkInTent(server, 1)) {
        Vector3f newTentPos = server->protocol.ctf.base[1];
        newTentPos.z -= 1;
        SendMoveObject(server, 1 + 2, 1, newTentPos);
        server->protocol.ctf.base[1] = newTentPos;
    } else if (checkInIntel(server, 1)) {
        Vector3f newIntelPos = server->protocol.ctf.intel[1];
        newIntelPos.z -= 1;
        SendMoveObject(server, 1, 1, newIntelPos);
        server->protocol.ctf.intel[1] = newIntelPos;
    } else if (checkInIntel(server, 0)) {
        Vector3f newIntelPos = server->protocol.ctf.intel[0];
        newIntelPos.z -= 1;
        SendMoveObject(server, 0, 0, newIntelPos);
        server->protocol.ctf.intel[0] = newIntelPos;
    }
}

uint8 checkUnderTent(Server* server, uint8 team)
{
    uint8 count = 0;
    for (int x = server->protocol.ctf.base[team].x - 1; x <= server->protocol.ctf.base[team].x; x++) {
        for (int y = server->protocol.ctf.base[team].y - 1; y <= server->protocol.ctf.base[team].y; y++) {
            if (mapvxlIsSolid(&server->map.map, x, y, server->protocol.ctf.base[team].z) == 0) {
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
                      server->protocol.ctf.intel[team].x,
                      server->protocol.ctf.intel[team].y,
                      server->protocol.ctf.intel[team].z) == 0)
    {
        ret = 1;
    }
    return ret;
}

uint8 checkPlayerOnIntel(Server* server, uint8 playerID, uint8 team)
{
    uint8    ret       = 0;
    Vector3f playerPos = server->player[playerID].movement.position;
    Vector3f intelPos  = server->protocol.ctf.intel[team];
    if ((int) playerPos.y == (int) intelPos.y &&
        ((int) playerPos.z + 3 == (int) intelPos.z ||
         (server->player[playerID].crouching && (int) playerPos.z + 2 == (int) intelPos.z)) &&
        (int) playerPos.x == (int) intelPos.x)
    {
        ret = 1;
    }
    return ret;
}

uint8 checkPlayerInTent(Server* server, uint8 playerID)
{
    uint8    ret       = 0;
    Vector3f playerPos = server->player[playerID].movement.position;
    Vector3f tentPos   = server->protocol.ctf.base[server->player[playerID].team];
    if (((int) playerPos.z + 3 == (int) tentPos.z ||
         (server->player[playerID].crouching && (int) playerPos.z + 2 == (int) tentPos.z)) &&
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
    Vector3f intelPos = server->protocol.ctf.intel[team];
    if ((int) itemPos.y == (int) intelPos.y && ((int) itemPos.z == (int) intelPos.z) &&
        (int) itemPos.x == (int) intelPos.x) {
        ret = 1;
    }
    return ret;
}

uint8 checkItemInTent(Server* server, uint8 team, Vector3f itemPos)
{
    uint8    ret     = 0;
    Vector3f tentPos = server->protocol.ctf.base[team];
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
    Vector3f checkPos = server->protocol.ctf.base[team];
    checkPos.z--;
    if (mapvxlIsSolid(&server->map.map, (int) checkPos.x, (int) checkPos.y, (int) checkPos.z)) {
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
    Vector3f checkPos = server->protocol.ctf.intel[team];
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

void handleIntel(Server* server, uint8 playerID)
{
    uint8 team;
    if (server->player[playerID].team == 0) {
        team = 1;
    } else {
        team = 0;
    }
    if (server->player[playerID].team != TEAM_SPECTATOR) {
        time_t timeNow = time(NULL);
        if (server->player[playerID].hasIntel == 0) {

            if (checkPlayerOnIntel(server, playerID, team) && (!server->protocol.ctf.intelHeld[team])) {
                SendIntelPickup(server, playerID);
            } else if (checkPlayerInTent(server, playerID) &&
                       timeNow - server->player[playerID].timers.sinceLastBaseEnterRestock >= 15)
            {
                SendRestock(server, playerID);
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
                server->protocol.ctf.score[server->player[playerID].team]++;
                uint8 winning = 0;
                if (server->protocol.ctf.score[server->player[playerID].team] >= server->protocol.ctf.scoreLimit) {
                    winning = 1;
                }
                SendIntelCapture(server, playerID, winning);
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
                SendRestock(server, playerID);
                server->player[playerID].timers.sinceLastBaseEnter = time(NULL);
                server->protocol.ctf.intel[team]                   = SetIntelTentSpawnPoint(server, team);
                SendMoveObject(server, team, team, server->protocol.ctf.intel[team]);
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
    for (int i = 0; i < 3; ++i) {
        if (server->player[playerID].grenade[i].sent) {
            move_grenade(server, playerID, i);
            if ((get_nanos() - server->player[playerID].grenade[i].timeSinceSent) / 1000000000.f >=
                server->player[playerID].grenade[i].fuse)
            {
                SendBlockAction(server,
                                playerID,
                                3,
                                server->player[playerID].grenade[i].position.x,
                                server->player[playerID].grenade[i].position.y,
                                server->player[playerID].grenade[i].position.z);
                for (int y = 0; y < server->protocol.maxPlayers; ++y) {
                    if (server->player[y].state == STATE_READY) {
                        int diffX =
                        fabsf(server->player[y].movement.position.x - server->player[playerID].grenade[i].position.x);
                        int diffY =
                        fabsf(server->player[y].movement.position.y - server->player[playerID].grenade[i].position.y);
                        int diffZ =
                        fabsf(server->player[y].movement.position.z - server->player[playerID].grenade[i].position.z);
                        Vector3f playerPos  = server->player[y].movement.position;
                        Vector3f grenadePos = server->player[y].grenade[i].position;
                        if (diffX < 16 && diffY < 16 && diffZ < 16 &&
                            can_see(
                            server, playerPos.x, playerPos.y, playerPos.z, grenadePos.x, grenadePos.y, grenadePos.z) &&
                            server->player[playerID].grenade[i].position.z < 62)
                        {
                            int value = 4096 / ((diffX * diffX) + (diffY * diffY) + (diffZ * diffZ));
                            sendHP(server, playerID, y, value, 1, 3, 5);
                        }
                    }
                }
                float x = server->player[playerID].grenade[i].position.x;
                float y = server->player[playerID].grenade[i].position.y;
                for (int z = server->player[playerID].grenade[i].position.z - 1;
                     z <= server->player[playerID].grenade[i].position.z + 1;
                     ++z)
                {
                    if (z < 62 && (x >= 0 && x <= MAP_MAX_X && x - 1 >= 0 && x - 1 <= MAP_MAX_X && x + 1 >= 0 && x + 1 <= MAP_MAX_X) &&
                        (y >= 0 && y <= MAP_MAX_Y && y - 1 >= 0 && y - 1 <= MAP_MAX_Y && y + 1 >= 0 && y + 1 <= MAP_MAX_Y))
                    {
                        mapvxlSetAir(&server->map.map, x - 1, y - 1, z);
                        mapvxlSetAir(&server->map.map, x, y - 1, z);
                        mapvxlSetAir(&server->map.map, x + 1, y - 1, z);
                        mapvxlSetAir(&server->map.map, x - 1, y, z);
                        mapvxlSetAir(&server->map.map, x, y, z);
                        mapvxlSetAir(&server->map.map, x + 1, y, z);
                        mapvxlSetAir(&server->map.map, x - 1, y + 1, z);
                        mapvxlSetAir(&server->map.map, x, y + 1, z);
                        mapvxlSetAir(&server->map.map, x + 1, y + 1, z);
                        Vector3i pos;
                        pos.x = server->player[playerID].grenade[i].position.x;
                        pos.y = server->player[playerID].grenade[i].position.y;
                        pos.z = server->player[playerID].grenade[i].position.z;

                        Vector3i* neigh = getGrenadeNeighbors(pos);
                        for (int index = 0; index < 54; ++index) {
                            if (vecValidPos(neigh[index])) {
                                checkNode(server, neigh[index]);
                            }
                        }
                    }
                }
                server->player[playerID].grenade[i].sent = 0;
                moveIntelAndTentDown(server);
            }
        }
    }
}

void updateMovementAndGrenades(Server* server)
{
    float dt = 1.0f / 60.0f;
    server->globalTimers.timeSinceStartSimulated += dt;
    set_globals(server->globalTimers.timeSinceStartSimulated, dt);
    for (int playerID = 0; playerID < server->protocol.maxPlayers; playerID++) {
        if (server->player[playerID].state == STATE_READY) {
            long falldamage = 0;
            falldamage      = move_player(server, playerID);
            if (falldamage > 0) {
                sendHP(server, playerID, playerID, falldamage, 0, 4, 5);
            }
            handleIntel(server, playerID);
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
        server->player[playerID].movement.position.z = mapvxlFindTopBlock(&server->map.map, server->player[playerID].movement.position.x, server->player[playerID].movement.position.y) - 2.36f;

        server->player[playerID].movement.forwardOrientation.x = 0.f;
        server->player[playerID].movement.forwardOrientation.y = 0.f;
        server->player[playerID].movement.forwardOrientation.z = 0.f;

        printf("respawn: %f %f %f\n",
               server->player[playerID].movement.position.x,
               server->player[playerID].movement.position.y,
               server->player[playerID].movement.position.z);
    }
}

void sendServerNotice(Server* server, uint8 playerID, char* message)
{
    uint32      packetSize = 3 + strlen(message);
    ENetPacket* packet     = enet_packet_create(NULL, packetSize, ENET_PACKET_FLAG_RELIABLE);
    DataStream  stream     = {packet->data, packet->dataLength, 0};
    WriteByte(&stream, PACKET_TYPE_CHAT_MESSAGE);
    WriteByte(&stream, 33);
    WriteByte(&stream, 0);
    WriteArray(&stream, message, strlen(message));
    enet_peer_send(server->player[playerID].peer, 0, packet);
}

void broadcastServerNotice(Server* server, char* message)
{
    uint32      packetSize = 3 + strlen(message);
    ENetPacket* packet     = enet_packet_create(NULL, packetSize, ENET_PACKET_FLAG_RELIABLE);
    DataStream  stream     = {packet->data, packet->dataLength, 0};
    WriteByte(&stream, PACKET_TYPE_CHAT_MESSAGE);
    WriteByte(&stream, 33);
    WriteByte(&stream, 0);
    WriteArray(&stream, message, strlen(message));
    for (int player = 0; player < server->protocol.maxPlayers; ++player) {
        if (isPastJoinScreen(server, player)) {
            enet_peer_send(server->player[player].peer, 0, packet);
        }
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
            if (playerToPlayerVisible(server, playerID, i)) {
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
            if (playerToPlayerVisible(server, playerID, i)) {
                if (enet_peer_send(server->player[i].peer, 0, packet) == 0) {
                    sent = 1;
                }
            }
        }
    }
    return sent;
}

uint8 diffIsOlderThen(time_t timeNow, time_t* timeBefore, time_t timeDiff)
{
    if (timeNow - *timeBefore >= timeDiff) {
        *timeBefore = timeNow;
        return 1;
    } else {
        return 0;
    }
}

uint8 diffIsOlderThenDontUpdate(time_t timeNow, time_t timeBefore, time_t timeDiff)
{
    if (timeNow - timeBefore >= timeDiff) {
        return 1;
    } else {
        return 0;
    }
}
