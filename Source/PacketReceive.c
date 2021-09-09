// Copyright DarkNeutrino 2021
#include "ColorConversion.h"
#include "Enums.h"
#include "Map.h"
#include "Packets.h"
#include "Protocol.h"
#include "Structs.h"

#include <DataStream.h>
#include <Queue.h>
#include <Types.h>
#include <enet/enet.h>
#include <libmapvxl/libmapvxl.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

void reorient_player(Server* server, uint8 playerID, Vector3f* orientation);
int  validate_hit(Vector3f shooter, Vector3f orientation, Vector3f otherPos, float tolerance);
long cast_ray(Server* server,
              float   x0,
              float   y0,
              float   z0,
              float   x1,
              float   y1,
              float   z1,
              float   length,
              long*   x,
              long*   y,
              long*   z);

static unsigned long long get_nanos(void)
{
    struct timespec ts;
    timespec_get(&ts, TIME_UTC);
    return (unsigned long long) ts.tv_sec * 1000000000L + ts.tv_nsec;
}

void ReceiveGrenadePacket(Server* server, uint8 playerID, DataStream* data)
{
    for (int i = 0; i < 3; ++i) {
        if (server->player[playerID].grenade[i].sent == 0) {
            server->player[playerID].grenade[i].fuse       = ReadFloat(data);
            server->player[playerID].grenade[i].position.x = ReadFloat(data);
            server->player[playerID].grenade[i].position.y = ReadFloat(data);
            server->player[playerID].grenade[i].position.z = ReadFloat(data);
            server->player[playerID].grenade[i].velocity.x = ReadFloat(data);
            server->player[playerID].grenade[i].velocity.y = ReadFloat(data);
            server->player[playerID].grenade[i].velocity.z = ReadFloat(data);
            if (vecfValidPos(server->player[playerID].grenade[i].position)) {
                SendGrenade(server,
                            playerID,
                            server->player[playerID].grenade[i].fuse,
                            server->player[playerID].grenade[i].position,
                            server->player[playerID].grenade[i].velocity);
                server->player[playerID].grenade[i].sent          = 1;
                server->player[playerID].grenade[i].timeSinceSent = get_nanos();
            }
            break;
        }
    }
}

// hitPlayerID is the player that got shot
// playerID is the player who fired.
void ReceiveHitPacket(Server* server, uint8 playerID, uint8 hitPlayerID, uint8 hitType)
{
    Vector3f shotPos    = server->player[playerID].movement.position;
    Vector3f shotEyePos = server->player[playerID].movement.eyePos;
    Vector3f hitPos     = server->player[hitPlayerID].movement.position;
    Vector3f shotOrien  = server->player[playerID].movement.forwardOrientation;
    float    distance   = DistanceIn2D(shotPos, hitPos);
    long     x, y, z;
    if (server->player[playerID].alive && server->player[hitPlayerID].alive &&
        (server->player[playerID].team != server->player[hitPlayerID].team ||
         server->player[playerID].allowTeamKilling) &&
        (server->player[playerID].allowKilling && server->globalAK) && validate_hit(shotPos, shotOrien, hitPos, 5) &&
        (cast_ray(server,
                  shotEyePos.x,
                  shotEyePos.y,
                  shotEyePos.z,
                  shotOrien.x,
                  shotOrien.y,
                  shotOrien.z,
                  distance,
                  &x,
                  &y,
                  &z) == 0 ||
         cast_ray(server,
                  shotEyePos.x,
                  shotEyePos.y,
                  shotEyePos.z - 1,
                  shotOrien.x,
                  shotOrien.y,
                  shotOrien.z,
                  distance,
                  &x,
                  &y,
                  &z) == 0))
    {
        switch (server->player[playerID].weapon) {
            case WEAPON_RIFLE:
            {
                switch (hitType) {
                    case HIT_TYPE_HEAD:
                    {
                        sendKillPacket(server, playerID, hitPlayerID, 1, 5, 0);
                        break;
                    }
                    case HIT_TYPE_TORSO:
                    {
                        sendHP(server, playerID, hitPlayerID, 49, 1, 0, 5);
                        break;
                    }
                    case HIT_TYPE_ARMS:
                    {
                        sendHP(server, playerID, hitPlayerID, 33, 1, 0, 5);
                        break;
                    }
                    case HIT_TYPE_LEGS:
                    {
                        sendHP(server, playerID, hitPlayerID, 33, 1, 0, 5);
                        break;
                    }
                }
                break;
            }
            case WEAPON_SMG:
            {
                switch (hitType) {
                    case HIT_TYPE_HEAD:
                    {
                        sendHP(server, playerID, hitPlayerID, 75, 1, 1, 5);
                        break;
                    }
                    case HIT_TYPE_TORSO:
                    {
                        sendHP(server, playerID, hitPlayerID, 29, 1, 0, 5);
                        break;
                    }
                    case HIT_TYPE_ARMS:
                    {
                        sendHP(server, playerID, hitPlayerID, 18, 1, 0, 5);
                        break;
                    }
                    case HIT_TYPE_LEGS:
                    {
                        sendHP(server, playerID, hitPlayerID, 18, 1, 0, 5);
                        break;
                    }
                }
                break;
            }
            case WEAPON_SHOTGUN:
            {
                switch (hitType) {
                    case HIT_TYPE_HEAD:
                    {
                        sendHP(server, playerID, hitPlayerID, 37, 1, 1, 5);
                        break;
                    }
                    case HIT_TYPE_TORSO:
                    {
                        sendHP(server, playerID, hitPlayerID, 27, 1, 0, 5);
                        break;
                    }
                    case HIT_TYPE_ARMS:
                    {
                        sendHP(server, playerID, hitPlayerID, 16, 1, 0, 5);
                        break;
                    }
                    case HIT_TYPE_LEGS:
                    {
                        sendHP(server, playerID, hitPlayerID, 16, 1, 0, 5);
                        break;
                    }
                }
                break;
            }
        }
        if (hitType == HIT_TYPE_MELEE) {
            sendHP(server, playerID, hitPlayerID, 80, 1, 2, 5);
        }
    }
}

void ReceiveOrientationData(Server* server, uint8 playerID, DataStream* data)
{
    float x, y, z;
    x            = ReadFloat(data);
    y            = ReadFloat(data);
    z            = ReadFloat(data);
    float length = 1 / sqrt((x * x) + (y * y) + (z * z));
    // Normalize the vectors as soon as we get them
    server->player[playerID].movement.forwardOrientation.x = x * length;
    server->player[playerID].movement.forwardOrientation.y = y * length;
    server->player[playerID].movement.forwardOrientation.z = z * length;
    reorient_player(server, playerID, &server->player[playerID].movement.forwardOrientation);
}

void ReceiveInputData(Server* server, uint8 playerID, DataStream* data)
{
    uint8 bits[8];
    uint8 mask = 1;
    uint8 ID;
    ID = ReadByte(data);
    if (playerID != ID) {
        printf("Assigned ID: %d doesnt match sent ID: %d in Input packet\n", playerID, ID);
    } else if (server->player[playerID].state == STATE_READY) {
        server->player[playerID].input = ReadByte(data);
        for (int i = 0; i < 8; i++) {
            bits[i] = (server->player[playerID].input >> i) & mask;
        }
        server->player[playerID].movForward   = bits[0];
        server->player[playerID].movBackwards = bits[1];
        server->player[playerID].movLeft      = bits[2];
        server->player[playerID].movRight     = bits[3];
        server->player[playerID].jumping      = bits[4];
        server->player[playerID].crouching    = bits[5];
        server->player[playerID].sneaking     = bits[6];
        server->player[playerID].sprinting    = bits[7];
        SendInputData(server, playerID);
    }
}

void ReceivePositionData(Server* server, uint8 playerID, DataStream* data)
{
    float x, y, z;
    x = ReadFloat(data);
    y = ReadFloat(data);
    z = ReadFloat(data);
    if (validPos(x, y, z)) {
        server->player[playerID].movement.position.x = x;
        server->player[playerID].movement.position.y = y;
        server->player[playerID].movement.position.z = z;
    }
}

void ReceiveExistingPlayer(Server* server, uint8 playerID, DataStream* data)
{
    uint8 ID = ReadByte(data);
    if (playerID != ID) {
        printf("Assigned ID: %d doesnt match sent ID: %d in existing player packet\n", playerID, ID);
    }

    server->player[playerID].team   = ReadByte(data);
    server->player[playerID].weapon = ReadByte(data);
    server->player[playerID].item   = ReadByte(data);
    server->player[playerID].kills  = ReadInt(data);

    ReadColor3i(data, server->player[playerID].color);
    server->player[playerID].ups = 60;

    uint32 length = DataLeft(data);
    if (length > 16) {
        WARNING("name too long");
        length = 16;
    } else {
        server->player[playerID].name[length] = '\0';
        ReadArray(data, server->player[playerID].name, length);
        int count = 0;
        for (uint8 i = 0; i < server->protocol.maxPlayers; i++) {
            if (isPastJoinScreen(server, i) && i != playerID) {
                if (strcmp(server->player[playerID].name, server->player[i].name) == 0) {
                    count++;
                }
            }
        }
        if (count > 0) {
            char idChar[4];
            sprintf(idChar, "%d", playerID);
            strcat(server->player[playerID].name, idChar);
        }
    }
    switch (server->player[playerID].weapon) {
        case 0:
            server->player[playerID].weaponReserve = 50;
            server->player[playerID].weaponClip    = 10;
            break;
        case 1:
            server->player[playerID].weaponReserve = 120;
            server->player[playerID].weaponClip    = 30;
            break;
        case 2:
            server->player[playerID].weaponReserve = 48;
            server->player[playerID].weaponClip    = 6;
            break;
    }
    server->player[playerID].state = STATE_SPAWNING;
}

void OnPacketReceived(Server* server, uint8 playerID, DataStream* data, ENetEvent event)
{
    PacketID type = (PacketID) ReadByte(data);
    switch (type) {
        case PACKET_TYPE_EXISTING_PLAYER:
            ReceiveExistingPlayer(server, playerID, data);
            break;
        case PACKET_TYPE_POSITION_DATA:
            ReceivePositionData(server, playerID, data);
            break;
        case PACKET_TYPE_ORIENTATION_DATA:
            ReceiveOrientationData(server, playerID, data);
            break;
        case PACKET_TYPE_INPUT_DATA:
            ReceiveInputData(server, playerID, data);
            break;
        case PACKET_TYPE_CHAT_MESSAGE:
            handleAndSendMessage(event, data, server, playerID);
            break;
        case PACKET_TYPE_BLOCK_ACTION:
        {
            uint8 ID = ReadByte(data);
            if (playerID != ID) {
                printf("Assigned ID: %d doesnt match sent ID: %d in block action packet\n", playerID, ID);
            }
            if (server->player[playerID].canBuild && server->globalAB) {
                uint8    actionType   = ReadByte(data);
                int      X            = ReadInt(data);
                int      Y            = ReadInt(data);
                int      Z            = ReadInt(data);
                Vector3f vectorBlock  = {X, Y, Z};
                Vector3f playerVector = server->player[playerID].movement.position;
                if ((server->player[playerID].blocks > 0) &&
                    ((server->player[playerID].item == 0 && (actionType == 1 || actionType == 2)) ||
                     (server->player[playerID].item == 1 && actionType == 0) ||
                     (server->player[playerID].item == 2 && actionType == 1)))
                {
                    if ((DistanceIn3D(vectorBlock, playerVector) <= 4 || server->player[playerID].item == 2) &&
                        vecfValidPos(vectorBlock)) {
                        switch (actionType) {
                            case 0:
                                mapvxlSetColor(
                                &server->map.map, X, Y, Z, color4iToInt(server->player[playerID].toolColor));
                                server->player[playerID].blocks--;
                                moveIntelAndTentUp(server);
                                break;

                            case 1:
                            {
                                Vector3i  position = {X, Y, Z};
                                Vector3i* neigh    = getNeighbors(position);
                                mapvxlSetAir(&server->map.map, position.x, position.y, position.z);
                                for (int i = 0; i < 6; ++i) {
                                    if (neigh[i].z < 62) {
                                        checkNode(server, neigh[i]);
                                    }
                                }
                                if (server->player[playerID].item != 2) {
                                    if (server->player[playerID].blocks < 50) {
                                        server->player[playerID].blocks++;
                                    }
                                }
                            } break;

                            case 2:
                                for (int z = Z - 1; z <= Z + 1; z++) {
                                    if (z < 62) {
                                        mapvxlSetAir(&server->map.map, X, Y, z);
                                        if (server->player[playerID].blocks < 50) {
                                            server->player[playerID].blocks++;
                                        }
                                        Vector3i  position = {X, Y, z};
                                        Vector3i* neigh    = getNeighbors(position);
                                        mapvxlSetAir(&server->map.map, position.x, position.y, position.z);
                                        for (int i = 0; i < 6; ++i) {
                                            if (neigh[i].z < 62) {
                                                checkNode(server, neigh[i]);
                                            }
                                        }
                                    }
                                }
                                break;
                        }
                        SendBlockAction(server, playerID, actionType, X, Y, Z);
                        moveIntelAndTentDown(server);
                    }
                } else {
                    printf("Player: #%d may be using BlockExploit with Item: %d and Action: %d\n",
                           playerID,
                           server->player[playerID].item,
                           actionType);
                }
            }
            break;
        }
        case PACKET_TYPE_BLOCK_LINE:
        {
            uint8 ID = ReadByte(data);
            if (playerID != ID) {
                printf("Assigned ID: %d doesnt match sent ID: %d in blockline packet\n", playerID, ID);
            }
            if (server->player[playerID].blocks > 0 && server->player[playerID].canBuild && server->globalAB &&
                server->player[playerID].item == 1)
            {
                vec3i start;
                vec3i end;
                start.x         = ReadInt(data);
                start.y         = ReadInt(data);
                start.z         = ReadInt(data);
                end.x           = ReadInt(data);
                end.y           = ReadInt(data);
                end.z           = ReadInt(data);
                Vector3f startF = {start.x, start.y, start.z};
                Vector3f endF   = {end.x, end.y, end.z};
                if (DistanceIn3D(endF, server->player[playerID].movement.position) <= 4 &&
                    DistanceIn3D(startF, server->player[playerID].locAtClick) <= 4 && vecfValidPos(startF) &&
                    vecfValidPos(endF))
                {
                    writeBlockLine(server, playerID, &start, &end);
                    moveIntelAndTentUp(server);
                    SendBlockLine(server, playerID, start, end);
                }
            }
            break;
        }
        case PACKET_TYPE_SET_TOOL:
        {
            uint8 ID   = ReadByte(data);
            uint8 tool = ReadByte(data);
            if (playerID != ID) {
                printf("Assigned ID: %d doesnt match sent ID: %d in set tool packet\n", playerID, ID);
            }
            server->player[playerID].item = tool;
            SendSetTool(server, playerID, tool);
            break;
        }
        case PACKET_TYPE_SET_COLOR:
        {
            uint8 ID = ReadByte(data);
            uint8 B  = ReadByte(data);
            uint8 G  = ReadByte(data);
            uint8 R  = ReadByte(data);
            uint8 A  = 0;

            if (playerID != ID) {
                printf("Assigned ID: %d doesnt match sent ID: %d in set color packet\n", playerID, ID);
            }

            server->player[playerID].toolColor[0] = A;
            server->player[playerID].toolColor[1] = R;
            server->player[playerID].toolColor[2] = G;
            server->player[playerID].toolColor[3] = B;
            SendSetColor(server, playerID, R, G, B);
            break;
        }

        case PACKET_TYPE_WEAPON_INPUT:
        {
            uint8 mask = 1;
            uint8 ID     = ReadByte(data);
            uint8 wInput = ReadByte(data);
            if (playerID != ID) {
                printf("Assigned ID: %d doesnt match sent ID: %d in weapon input packet\n", playerID, ID);
            }
            else if (server->player[playerID].state != STATE_READY) {
                return;
            }

            server->player[playerID].primary_fire   = wInput & mask;
            server->player[playerID].secondary_fire = (wInput >> 1) & mask;

            if (server->player[playerID].secondary_fire && server->player[playerID].item == 1) {
                server->player[playerID].locAtClick = server->player[playerID].movement.position;
            }

            else if (server->player[playerID].weaponClip >= 0)
            {
                SendWeaponInput(server, playerID, wInput);

                if (server->player[playerID].primary_fire) {
                    server->player[playerID].weaponClip--;
                }
            } else {
                // sendKillPacket(server, playerID, playerID, 0, 30, 0);
            }
            break;
        }

        case PACKET_TYPE_HIT_PACKET:
        {
            uint8 hitPlayerID = ReadByte(data);
            Hit   hitType     = ReadByte(data);
            ReceiveHitPacket(server, playerID, hitPlayerID, hitType);
            break;
        }
        case PACKET_TYPE_WEAPON_RELOAD:
        {
            uint8 ID      = ReadByte(data);
            uint8 reserve = ReadByte(data);
            uint8 clip    = ReadByte(data);
            if (playerID != ID) {
                printf("Assigned ID: %d doesnt match sent ID: %d in weapon reload packet\n", playerID, ID);
            }
            server->player[playerID].weaponReserve = reserve; // Temporary
            server->player[playerID].weaponClip    = clip;
            SendWeaponReload(server, playerID);
            break;
        }
        case PACKET_TYPE_CHANGE_TEAM:
        {
            uint8 ID                      = ReadByte(data);
            server->player[playerID].team = ReadByte(data);
            if (playerID != ID) {
                printf("Assigned ID: %d doesnt match sent ID: %d in change team packet\n", playerID, ID);
            }
            sendKillPacket(server, playerID, playerID, 5, 5, 0);
            server->player[playerID].state = STATE_WAITING_FOR_RESPAWN;
            break;
        }
        case PACKET_TYPE_CHANGE_WEAPON:
        {
            uint8 ID                        = ReadByte(data);
            server->player[playerID].weapon = ReadByte(data);
            if (playerID != ID) {
                printf("Assigned ID: %d doesnt match sent ID: %d in change weapon packet\n", playerID, ID);
            }
            sendKillPacket(server, playerID, playerID, 6, 5, 0);
            server->player[playerID].state = STATE_WAITING_FOR_RESPAWN;
            break;
        }
        case PACKET_TYPE_GRENADE_PACKET:
        {
            uint8 ID = ReadByte(data);
            if (playerID != ID) {
                printf("Assigned ID: %d doesnt match sent ID: %d in grenade packet\n", playerID, ID);
            }
            if (server->player[playerID].grenades > 0) {
                ReceiveGrenadePacket(server, playerID, data);
                server->player[playerID].grenades--;
            }
            break;
        }
        default:
            printf("unhandled input, id %u, code %u\n", playerID, type);
            break;
    }
}
