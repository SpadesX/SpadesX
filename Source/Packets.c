// Copyright DarkNeutrino 2021
#include "Commands.h"
#include "Protocol.h"
#include "Structs.h"

#include <Compress.h>
#include <DataStream.h>
#include <Enums.h>
#include <Line.h>
#include <Queue.h>
#include <Types.h>
#include <bsd/string.h>
#include <ctype.h>
#include <enet/enet.h>
#include <libmapvxl/libmapvxl.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

void reorient_player(Server* server, uint8 playerID, Vector3f* orientation);
int  validate_hit(Vector3f shooter, Vector3f orientation, Vector3f otherPos, float tolerance);

static unsigned long long get_nanos(void)
{
    struct timespec ts;
    timespec_get(&ts, TIME_UTC);
    return (unsigned long long) ts.tv_sec * 1000000000L + ts.tv_nsec;
}

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

inline static uint8 allowShot(Server*  server,
                              uint8    playerID,
                              uint8    hitPlayerID,
                              time_t   timeNow,
                              float    distance,
                              long*    x,
                              long*    y,
                              long*    z,
                              Vector3f shotPos,
                              Vector3f shotOrien,
                              Vector3f hitPos,
                              Vector3f shotEyePos)
{
    uint8 ret = 0;
    if (server->player[playerID].primary_fire &&
        ((server->player[playerID].item == 0 &&
          diffIsOlderThen(timeNow, &server->player[playerID].timers.sinceLastShot, NANO_IN_MILLI * 100)) ||
         (server->player[playerID].item == 2 && server->player[playerID].weapon == 0 &&
          diffIsOlderThen(timeNow, &server->player[playerID].timers.sinceLastShot, NANO_IN_MILLI * 500)) ||
         (server->player[playerID].item == 2 && server->player[playerID].weapon == 1 &&
          diffIsOlderThen(timeNow, &server->player[playerID].timers.sinceLastShot, NANO_IN_MILLI * 110)) ||
         (server->player[playerID].item == 2 && server->player[playerID].weapon == 2 &&
          diffIsOlderThen(timeNow, &server->player[playerID].timers.sinceLastShot, NANO_IN_MILLI * 1000))) &&
        server->player[playerID].alive && server->player[hitPlayerID].alive &&
        (server->player[playerID].team != server->player[hitPlayerID].team ||
         server->player[playerID].allowTeamKilling) &&
        (server->player[playerID].allowKilling && server->globalAK) && validate_hit(shotPos, shotOrien, hitPos, 5) &&
        (cast_ray(
         server, shotEyePos.x, shotEyePos.y, shotEyePos.z, shotOrien.x, shotOrien.y, shotOrien.z, distance, x, y, z) ==
         0 ||
         cast_ray(server,
                  shotEyePos.x,
                  shotEyePos.y,
                  shotEyePos.z - 1,
                  shotOrien.x,
                  shotOrien.y,
                  shotOrien.z,
                  distance,
                  x,
                  y,
                  z) == 0))
    {
        ret = 1;
    }
    return ret;
}

void SendRestock(Server* server, uint8 playerID)
{
    ENetPacket* packet = enet_packet_create(NULL, 2, ENET_PACKET_FLAG_RELIABLE);
    DataStream  stream = {packet->data, packet->dataLength, 0};
    WriteByte(&stream, PACKET_TYPE_RESTOCK);
    WriteByte(&stream, playerID);
    enet_peer_send(server->player[playerID].peer, 0, packet);
}

void SendMoveObject(Server* server, uint8 object, uint8 team, Vector3f pos)
{
    ENetPacket* packet = enet_packet_create(NULL, 15, ENET_PACKET_FLAG_RELIABLE);
    DataStream  stream = {packet->data, packet->dataLength, 0};
    WriteByte(&stream, PACKET_TYPE_MOVE_OBJECT);
    WriteByte(&stream, object);
    WriteByte(&stream, team);
    WriteFloat(&stream, pos.x);
    WriteFloat(&stream, pos.y);
    WriteFloat(&stream, pos.z);
    for (int player = 0; player < server->protocol.maxPlayers; ++player) {
        if (isPastStateData(server, player)) {
            enet_peer_send(server->player[player].peer, 0, packet);
        }
    }
}

void SendIntelCapture(Server* server, uint8 playerID, uint8 winning)
{
    uint8 team;
    if (server->player[playerID].team == 0) {
        team = 1;
    } else {
        team = 0;
    }
    if (server->player[playerID].hasIntel == 0 || server->protocol.gameMode.intelHeld[team] == 0) {
        return;
    }
    ENetPacket* packet = enet_packet_create(NULL, 3, ENET_PACKET_FLAG_RELIABLE);
    DataStream  stream = {packet->data, packet->dataLength, 0};
    WriteByte(&stream, PACKET_TYPE_INTEL_CAPTURE);
    WriteByte(&stream, playerID);
    WriteByte(&stream, winning);
    server->player[playerID].hasIntel         = 0;
    server->protocol.gameMode.intelHeld[team] = 0;
    for (int player = 0; player < server->protocol.maxPlayers; ++player) {
        if (isPastStateData(server, player)) {
            enet_peer_send(server->player[player].peer, 0, packet);
        }
    }
}

void SendIntelPickup(Server* server, uint8 playerID)
{

    uint8 team;
    if (server->player[playerID].team == 0) {
        team = 1;
    } else {
        team = 0;
    }
    if (server->player[playerID].hasIntel == 1 || server->protocol.gameMode.intelHeld[team] == 1) {
        return;
    }
    ENetPacket* packet = enet_packet_create(NULL, 2, ENET_PACKET_FLAG_RELIABLE);
    DataStream  stream = {packet->data, packet->dataLength, 0};
    WriteByte(&stream, PACKET_TYPE_INTEL_PICKUP);
    WriteByte(&stream, playerID);
    server->player[playerID].hasIntel                                        = 1;
    server->protocol.gameMode.playerIntelTeam[server->player[playerID].team] = playerID;
    server->protocol.gameMode.intelHeld[team]                                = 1;

    for (int player = 0; player < server->protocol.maxPlayers; ++player) {
        if (isPastStateData(server, player)) {
            enet_peer_send(server->player[player].peer, 0, packet);
        }
    }
}

void SendIntelDrop(Server* server, uint8 playerID)
{
    uint8 team;
    if (server->player[playerID].team == 0) {
        team = 1;
    } else {
        team = 0;
    }
    if (server->player[playerID].hasIntel == 0 || server->protocol.gameMode.intelHeld[team] == 0) {
        return;
    }
    ENetPacket* packet = enet_packet_create(NULL, 14, ENET_PACKET_FLAG_RELIABLE);
    DataStream  stream = {packet->data, packet->dataLength, 0};
    WriteByte(&stream, PACKET_TYPE_INTEL_DROP);
    WriteByte(&stream, playerID);
    if (server->protocol.currentGameMode == GAME_MODE_BABEL) {
        WriteFloat(&stream, MAP_MAX_X / 2);
        WriteFloat(&stream, MAP_MAX_Y / 2);
        WriteFloat(&stream, (float) mapvxlFindTopBlock(&server->map.map, MAP_MAX_X / 2, MAP_MAX_Y / 2));

        server->protocol.gameMode.intel[team].x = MAP_MAX_X / 2;
        server->protocol.gameMode.intel[team].y = MAP_MAX_Y / 2;
        server->protocol.gameMode.intel[team].z = mapvxlFindTopBlock(&server->map.map, MAP_MAX_X / 2, MAP_MAX_Y / 2);
        server->protocol.gameMode.intel[server->player[playerID].team] = server->protocol.gameMode.intel[team];
        SendMoveObject(
        server, server->player[playerID].team, server->player[playerID].team, server->protocol.gameMode.intel[team]);
    } else {
        WriteFloat(&stream, server->player[playerID].movement.position.x);
        WriteFloat(&stream, server->player[playerID].movement.position.y);
        WriteFloat(&stream,
                   (float) mapvxlFindTopBlock(&server->map.map,
                                              server->player[playerID].movement.position.x,
                                              server->player[playerID].movement.position.y));

        server->protocol.gameMode.intel[team].x = (int) server->player[playerID].movement.position.x;
        server->protocol.gameMode.intel[team].y = (int) server->player[playerID].movement.position.y;
        server->protocol.gameMode.intel[team].z = mapvxlFindTopBlock(
        &server->map.map, server->player[playerID].movement.position.x, server->player[playerID].movement.position.y);
    }
    server->player[playerID].hasIntel                                        = 0;
    server->protocol.gameMode.playerIntelTeam[server->player[playerID].team] = 32;
    server->protocol.gameMode.intelHeld[team]                                = 0;

    printf("Dropping intel at X: %d, Y: %d, Z: %d\n",
           (int) server->protocol.gameMode.intel[team].x,
           (int) server->protocol.gameMode.intel[team].y,
           (int) server->protocol.gameMode.intel[team].z);
    for (int player = 0; player < server->protocol.maxPlayers; ++player) {
        if (isPastStateData(server, player)) {
            enet_peer_send(server->player[player].peer, 0, packet);
        }
    }
}

void SendGrenade(Server* server, uint8 playerID, float fuse, Vector3f position, Vector3f velocity)
{
    ENetPacket* packet = enet_packet_create(NULL, 30, ENET_PACKET_FLAG_RELIABLE);
    DataStream  stream = {packet->data, packet->dataLength, 0};
    WriteByte(&stream, PACKET_TYPE_GRENADE_PACKET);
    WriteByte(&stream, playerID);
    WriteFloat(&stream, fuse);
    WriteFloat(&stream, position.x);
    WriteFloat(&stream, position.y);
    WriteFloat(&stream, position.z);
    WriteFloat(&stream, velocity.x);
    WriteFloat(&stream, velocity.y);
    WriteFloat(&stream, velocity.z);
    if (SendPacketExceptSender(server, packet, playerID) == 0) {
        enet_packet_destroy(packet);
    }
}

void SendPlayerLeft(Server* server, uint8 playerID)
{
    printf("Player %s disconnected\n", server->player[playerID].name);
    for (uint8 i = 0; i < server->protocol.maxPlayers; ++i) {
        if (i != playerID && isPastStateData(server, i)) {
            ENetPacket* packet = enet_packet_create(NULL, 2, ENET_PACKET_FLAG_RELIABLE);
            DataStream  stream = {packet->data, packet->dataLength, 0};
            WriteByte(&stream, PACKET_TYPE_PLAYER_LEFT);
            WriteByte(&stream, playerID);

            if (enet_peer_send(server->player[i].peer, 0, packet) != 0) {
                WARNING("failed to send player left event\n");
            }
        }
    }
}

void SendWeaponReload(Server* server, uint8 playerID)
{
    ENetPacket* packet = enet_packet_create(NULL, 4, ENET_PACKET_FLAG_RELIABLE);
    DataStream  stream = {packet->data, packet->dataLength, 0};
    WriteByte(&stream, PACKET_TYPE_WEAPON_RELOAD);
    WriteByte(&stream, playerID);
    WriteByte(&stream, server->player[playerID].weaponClip);
    WriteByte(&stream, server->player[playerID].weaponReserve);
    uint8 sendSucc = 0;
    for (int player = 0; player < server->protocol.maxPlayers; ++player) {
        if (isPastStateData(server, player)) {
            if (enet_peer_send(server->player[player].peer, 0, packet) == 0) {
                sendSucc = 1;
            }
        }
    }
    if (sendSucc == 0) {
        printf("destroying packet\n");
        enet_packet_destroy(packet);
    }
}

void SendWeaponInput(Server* server, uint8 playerID, uint8 wInput)
{
    ENetPacket* packet = enet_packet_create(NULL, 3, ENET_PACKET_FLAG_RELIABLE);
    DataStream  stream = {packet->data, packet->dataLength, 0};
    WriteByte(&stream, PACKET_TYPE_WEAPON_INPUT);
    WriteByte(&stream, playerID);
    if (server->player[playerID].sprinting) {
        wInput = 0;
    }
    WriteByte(&stream, wInput);
    if (SendPacketExceptSender(server, packet, playerID) == 0) {
        enet_packet_destroy(packet);
    }
}

void SendSetColor(Server* server, uint8 playerID, uint8 R, uint8 G, uint8 B)
{
    ENetPacket* packet = enet_packet_create(NULL, 5, ENET_PACKET_FLAG_RELIABLE);
    DataStream  stream = {packet->data, packet->dataLength, 0};
    WriteByte(&stream, PACKET_TYPE_SET_COLOR);
    WriteByte(&stream, playerID);
    WriteByte(&stream, B);
    WriteByte(&stream, G);
    WriteByte(&stream, R);
    if (SendPacketExceptSender(server, packet, playerID) == 0) {
        enet_packet_destroy(packet);
    }
}

void SendSetTool(Server* server, uint8 playerID, uint8 tool)
{
    ENetPacket* packet = enet_packet_create(NULL, 3, ENET_PACKET_FLAG_RELIABLE);
    DataStream  stream = {packet->data, packet->dataLength, 0};
    WriteByte(&stream, PACKET_TYPE_SET_TOOL);
    WriteByte(&stream, playerID);
    WriteByte(&stream, tool);
    if (SendPacketExceptSender(server, packet, playerID) == 0) {
        enet_packet_destroy(packet);
    }
}

void SendBlockLine(Server* server, uint8 playerID, Vector3i start, Vector3i end)
{
    ENetPacket* packet = enet_packet_create(NULL, 26, ENET_PACKET_FLAG_RELIABLE);
    DataStream  stream = {packet->data, packet->dataLength, 0};
    WriteByte(&stream, PACKET_TYPE_BLOCK_LINE);
    WriteByte(&stream, playerID);
    WriteInt(&stream, start.x);
    WriteInt(&stream, start.y);
    WriteInt(&stream, start.z);
    WriteInt(&stream, end.x);
    WriteInt(&stream, end.y);
    WriteInt(&stream, end.z);
    for (int player = 0; player < server->protocol.maxPlayers; ++player) {
        if (isPastStateData(server, player)) {
            enet_peer_send(server->player[player].peer, 0, packet);
        }
    }
}

void SendBlockAction(Server* server, uint8 playerID, uint8 actionType, int X, int Y, int Z)
{
    ENetPacket* packet = enet_packet_create(NULL, 15, ENET_PACKET_FLAG_RELIABLE);
    DataStream  stream = {packet->data, packet->dataLength, 0};
    WriteByte(&stream, PACKET_TYPE_BLOCK_ACTION);
    WriteByte(&stream, playerID);
    WriteByte(&stream, actionType);
    WriteInt(&stream, X);
    WriteInt(&stream, Y);
    WriteInt(&stream, Z);
    for (int player = 0; player < server->protocol.maxPlayers; ++player) {
        if (isPastStateData(server, player)) {
            enet_peer_send(server->player[player].peer, 0, packet);
        }
    }
}

void SendStateData(Server* server, uint8 playerID)
{
    ENetPacket* packet = enet_packet_create(NULL, 104, ENET_PACKET_FLAG_RELIABLE);
    DataStream  stream = {packet->data, packet->dataLength, 0};
    WriteByte(&stream, PACKET_TYPE_STATE_DATA);
    WriteByte(&stream, playerID);
    WriteColor3i(&stream, server->protocol.colorFog);
    WriteColor3i(&stream, server->protocol.colorTeam[0]);
    WriteColor3i(&stream, server->protocol.colorTeam[1]);
    WriteArray(&stream, server->protocol.nameTeam[0], 10);
    WriteArray(&stream, server->protocol.nameTeam[1], 10);
    if (server->protocol.currentGameMode == 0 || server->protocol.currentGameMode == 1) {
        WriteByte(&stream, server->protocol.currentGameMode);
    } else {
        WriteByte(&stream, 0);
    }

    // MODE CTF:

    WriteByte(&stream, server->protocol.gameMode.score[0]);   // SCORE TEAM A
    WriteByte(&stream, server->protocol.gameMode.score[1]);   // SCORE TEAM B
    WriteByte(&stream, server->protocol.gameMode.scoreLimit); // SCORE LIMIT

    server->protocol.gameMode.intelFlags = 0;

    if (server->protocol.gameMode.intelHeld[0]) {
        server->protocol.gameMode.intelFlags = INTEL_TEAM_B;
    } else if (server->protocol.gameMode.intelHeld[1]) {
        server->protocol.gameMode.intelFlags = INTEL_TEAM_A;
    } else if (server->protocol.gameMode.intelHeld[0] && server->protocol.gameMode.intelHeld[1]) {
        server->protocol.gameMode.intelFlags = INTEL_TEAM_BOTH;
    }

    WriteByte(&stream, server->protocol.gameMode.intelFlags); // INTEL FLAGS

    if ((server->protocol.gameMode.intelFlags & 1) == 0) {
        WriteByte(&stream, server->protocol.gameMode.playerIntelTeam[1]);
        for (int i = 0; i < 11; ++i) {
            WriteByte(&stream, 255);
        }
    } else {
        WriteVector3f(&stream, server->protocol.gameMode.intel[0]);
    }

    if ((server->protocol.gameMode.intelFlags & 2) == 0) {
        WriteByte(&stream, server->protocol.gameMode.playerIntelTeam[0]);
        for (int i = 0; i < 11; ++i) {
            WriteByte(&stream, 255);
        }
    } else {
        WriteVector3f(&stream, server->protocol.gameMode.intel[1]);
    }

    WriteVector3f(&stream, server->protocol.gameMode.base[0]);
    WriteVector3f(&stream, server->protocol.gameMode.base[1]);

    if (enet_peer_send(server->player[playerID].peer, 0, packet) == 0) {
        server->player[playerID].state = STATE_PICK_SCREEN;
    }
}

void SendInputData(Server* server, uint8 playerID)
{
    ENetPacket* packet = enet_packet_create(NULL, 3, ENET_PACKET_FLAG_RELIABLE);
    DataStream  stream = {packet->data, packet->dataLength, 0};
    WriteByte(&stream, PACKET_TYPE_INPUT_DATA);
    WriteByte(&stream, playerID);
    WriteByte(&stream, server->player[playerID].input);
    if (SendPacketExceptSenderDistCheck(server, packet, playerID) == 0) {
        enet_packet_destroy(packet);
    }
}

void sendKillPacket(Server* server,
                    uint8   killerID,
                    uint8   playerID,
                    uint8   killReason,
                    uint8   respawnTime,
                    uint8   makeInvisible)
{
    ENetPacket* packet = enet_packet_create(NULL, 5, ENET_PACKET_FLAG_RELIABLE);
    DataStream  stream = {packet->data, packet->dataLength, 0};
    WriteByte(&stream, PACKET_TYPE_KILL_ACTION);
    WriteByte(&stream, playerID);    // Player that died.
    WriteByte(&stream, killerID);    // Player that killed.
    WriteByte(&stream, killReason);  // Killing reason (1 is headshot)
    WriteByte(&stream, respawnTime); // Time before respawn happens
    for (int player = 0; player < server->protocol.maxPlayers; ++player) {
        uint8 isPast = isPastStateData(server, player);
        if ((makeInvisible && player != playerID && isPast) || (isPast && !makeInvisible)) {
            enet_peer_send(server->player[player].peer, 0, packet);
        }
    }
    if (!makeInvisible && server->player[playerID].isInvisible == 0) {
        if (killerID != playerID) {
            server->player[killerID].kills++;
        }
        server->player[playerID].deaths++;
        server->player[playerID].alive                     = 0;
        server->player[playerID].respawnTime               = respawnTime;
        server->player[playerID].timers.startOfRespawnWait = time(NULL);
        server->player[playerID].state                     = STATE_WAITING_FOR_RESPAWN;
        switch (server->player[playerID].weapon) {
            case 0:
                server->player[playerID].weaponReserve  = 50;
                server->player[playerID].weaponClip     = 10;
                server->player[playerID].defaultClip    = RIFLE_DEFAULT_CLIP;
                server->player[playerID].defaultReserve = RIFLE_DEFAULT_RESERVE;
                break;
            case 1:
                server->player[playerID].weaponReserve  = 120;
                server->player[playerID].weaponClip     = 30;
                server->player[playerID].defaultClip    = SMG_DEFAULT_CLIP;
                server->player[playerID].defaultReserve = SMG_DEFAULT_RESERVE;
                break;
            case 2:
                server->player[playerID].weaponReserve  = 48;
                server->player[playerID].weaponClip     = 6;
                server->player[playerID].defaultClip    = SHOTGUN_DEFAULT_CLIP;
                server->player[playerID].defaultReserve = SHOTGUN_DEFAULT_RESERVE;
                break;
        }
    }
    if (server->player[playerID].hasIntel) {
        SendIntelDrop(server, playerID);
    }
}

void sendHP(Server* server,
            uint8   playerID,
            uint8   hitPlayerID,
            long    HPChange,
            uint8   typeOfDamage,
            uint8   killReason,
            uint8   respawnTime)
{
    if ((server->player[playerID].allowKilling && server->globalAK && server->player[playerID].allowKilling &&
         server->player[playerID].alive) ||
        typeOfDamage == 0)
    {
        if (HPChange > server->player[hitPlayerID].HP) {
            HPChange = server->player[hitPlayerID].HP;
        }
        server->player[hitPlayerID].HP -= HPChange;
        if (server->player[hitPlayerID].HP < 0) // We should NEVER return true here. If we do stuff is really broken
            server->player[hitPlayerID].HP = 0;

        else if (server->player[hitPlayerID].HP > 100) // Same as above
            server->player[hitPlayerID].HP = 100;

        if (server->player[hitPlayerID].HP == 0) {
            server->player[hitPlayerID].alive = 0;
            sendKillPacket(server, playerID, hitPlayerID, killReason, respawnTime, 0);
        }

        else if (server->player[hitPlayerID].HP > 0 && server->player[hitPlayerID].HP < 100)
        {
            ENetPacket* packet = enet_packet_create(NULL, 15, ENET_PACKET_FLAG_RELIABLE);
            DataStream  stream = {packet->data, packet->dataLength, 0};
            WriteByte(&stream, PACKET_TYPE_SET_HP);
            WriteByte(&stream, server->player[hitPlayerID].HP);
            WriteByte(&stream, typeOfDamage);
            if (typeOfDamage != 0) {
                WriteFloat(&stream, server->player[playerID].movement.position.x);
                WriteFloat(&stream, server->player[playerID].movement.position.y);
                WriteFloat(&stream, server->player[playerID].movement.position.z);
            } else {
                WriteFloat(&stream, 0);
                WriteFloat(&stream, 0);
                WriteFloat(&stream, 0);
            }
            enet_peer_send(server->player[hitPlayerID].peer, 0, packet);
        }
    }
}

void SendPlayerState(Server* server, uint8 playerID, uint8 otherID)
{
    ENetPacket* packet = enet_packet_create(NULL, 28, ENET_PACKET_FLAG_RELIABLE);
    DataStream  stream = {packet->data, packet->dataLength, 0};
    WriteByte(&stream, PACKET_TYPE_EXISTING_PLAYER);
    WriteByte(&stream, otherID);                           // ID
    WriteByte(&stream, server->player[otherID].team);      // TEAM
    WriteByte(&stream, server->player[otherID].weapon);    // WEAPON
    WriteByte(&stream, server->player[otherID].item);      // HELD ITEM
    WriteInt(&stream, server->player[otherID].kills);      // KILLS
    WriteColor3i(&stream, server->player[otherID].color);  // COLOR
    WriteArray(&stream, server->player[otherID].name, 16); // NAME

    if (enet_peer_send(server->player[playerID].peer, 0, packet) != 0) {
        WARNING("failed to send player state\n");
    }
}

void SendVersionRequest(Server* server, uint8 playerID)
{
    ENetPacket* packet = enet_packet_create(NULL, 1, ENET_PACKET_FLAG_RELIABLE);
    DataStream  stream = {packet->data, packet->dataLength, 0};
    WriteByte(&stream, PACKET_TYPE_VERSION_REQUEST);
    enet_peer_send(server->player[playerID].peer, 0, packet);
}
void SendMapStart(Server* server, uint8 playerID)
{
    STATUS("sending map info");
    ENetPacket* packet = enet_packet_create(NULL, 5, ENET_PACKET_FLAG_RELIABLE);
    DataStream  stream = {packet->data, packet->dataLength, 0};
    WriteByte(&stream, PACKET_TYPE_MAP_START);
    WriteInt(&stream, server->map.compressedSize);
    if (enet_peer_send(server->player[playerID].peer, 0, packet) == 0) {
        server->player[playerID].state = STATE_LOADING_CHUNKS;

        // map
        uint8* out = (uint8*) malloc(1024 * 1024 * 10); // TODO: Make me dynamic based on changes to map
        mapvxlWriteMap(&server->map.map, out);
        server->map.compressedMap       = CompressData(out, server->map.mapSize, DEFAULT_COMPRESSOR_CHUNK_SIZE);
        server->player[playerID].queues = server->map.compressedMap;
        free(out);
    }
}

void SendMapChunks(Server* server, uint8 playerID)
{
    if (server->player[playerID].queues == NULL) {
        while (server->map.compressedMap) {
            server->map.compressedMap = Pop(server->map.compressedMap);
        }
        SendVersionRequest(server, playerID);
        server->player[playerID].state = STATE_JOINING;
        STATUS("loading chunks done");
    } else {
        ENetPacket* packet =
        enet_packet_create(NULL, server->player[playerID].queues->length + 1, ENET_PACKET_FLAG_RELIABLE);
        DataStream stream = {packet->data, packet->dataLength, 0};
        WriteByte(&stream, PACKET_TYPE_MAP_CHUNK);
        WriteArray(&stream, server->player[playerID].queues->block, server->player[playerID].queues->length);

        if (enet_peer_send(server->player[playerID].peer, 0, packet) == 0) {
            server->player[playerID].queues = server->player[playerID].queues->next;
        }
    }
}

void SendRespawnState(Server* server, uint8 playerID, uint8 otherID)
{
    ENetPacket* packet = enet_packet_create(NULL, 32, ENET_PACKET_FLAG_RELIABLE);
    DataStream  stream = {packet->data, packet->dataLength, 0};
    WriteByte(&stream, PACKET_TYPE_CREATE_PLAYER);
    WriteByte(&stream, otherID);                                       // ID
    WriteByte(&stream, server->player[otherID].weapon);                // WEAPON
    WriteByte(&stream, server->player[otherID].team);                  // TEAM
    WriteVector3f(&stream, server->player[otherID].movement.position); // X Y Z
    WriteArray(&stream, server->player[otherID].name, 16);             // NAME

    if (enet_peer_send(server->player[playerID].peer, 0, packet) != 0) {
        WARNING("failed to send player state\n");
    }
}

void SendRespawn(Server* server, uint8 playerID)
{
    for (uint8 i = 0; i < server->protocol.maxPlayers; ++i) {
        if (isPastJoinScreen(server, i)) {
            SendRespawnState(server, i, playerID);
        }
    }
    server->player[playerID].state = STATE_READY;
}

void handleAndSendMessage(ENetEvent event, DataStream* data, Server* server, uint8 player)
{
    uint32 packetSize = event.packet->dataLength + 1;
    uint8  ID         = ReadByte(data);
    int    meantfor   = ReadByte(data);
    uint32 length     = DataLeft(data);
    char*  message    = calloc(length + 1, sizeof(char));
    ReadArray(data, message, length);
    if (player != ID) {
        printf("Assigned ID: %d doesnt match sent ID: %d in message packet\n", player, ID);
    }
    printf("Player %s (%ld) to %d said: %s\n",
           server->player[player].name,
           (long) server->player[player].peer->data,
           meantfor,
           message);
    message[length]    = '\0';
    ENetPacket* packet = enet_packet_create(NULL, packetSize, ENET_PACKET_FLAG_RELIABLE);
    DataStream  stream = {packet->data, packet->dataLength, 0};
    WriteByte(&stream, PACKET_TYPE_CHAT_MESSAGE);
    WriteByte(&stream, player);
    WriteByte(&stream, meantfor);
    WriteArray(&stream, message, length);
    uint8 sent = 0;
    if (message[0] == '/') {
        handleCommands(server, player, message);
    } else {
        for (int playerID = 0; playerID < server->protocol.maxPlayers; ++playerID) {
            if (isPastJoinScreen(server, playerID) && !server->player[player].muted &&
                ((server->player[playerID].team == server->player[player].team && meantfor == 1) || meantfor == 0))
            {
                if (enet_peer_send(server->player[playerID].peer, 0, packet) == 0) {
                    sent = 1;
                }
            }
        }
    }
    free(message);
    if (sent == 0) {
        enet_packet_destroy(packet);
    }
}

void SendWorldUpdate(Server* server, uint8 playerID)
{
    ENetPacket* packet = enet_packet_create(NULL, 1 + (32 * 24), 0);
    DataStream  stream = {packet->data, packet->dataLength, 0};
    WriteByte(&stream, PACKET_TYPE_WORLD_UPDATE);

    for (uint8 j = 0; j < server->protocol.maxPlayers; ++j) {
        if (playerToPlayerVisible(server, playerID, j) && server->player[j].isInvisible == 0) {
            /*float    dt       = (get_nanos() - server->globalTimers.lastUpdateTime) / 1000000000.f;
            Vector3f position = {server->player[j].movement.velocity.x * dt + server->player[j].movement.position.x,
                                 server->player[j].movement.velocity.y * dt + server->player[j].movement.position.y,
                                 server->player[j].movement.velocity.z * dt + server->player[j].movement.position.z};
            WriteVector3f(&stream, position);*/
            WriteVector3f(&stream, server->player[j].movement.position);
            WriteVector3f(&stream, server->player[j].movement.forwardOrientation);
        } else {
            Vector3f empty;
            empty.x = 0;
            empty.y = 0;
            empty.z = 0;
            WriteVector3f(&stream, empty);
            WriteVector3f(&stream, empty);
        }
    }
    enet_peer_send(server->player[playerID].peer, 0, packet);
}

void SendPositionPacket(Server* server, uint8 playerID, float x, float y, float z)
{
    ENetPacket* packet = enet_packet_create(NULL, 13, ENET_PACKET_FLAG_RELIABLE);
    DataStream  stream = {packet->data, packet->dataLength, 0};
    WriteByte(&stream, PACKET_TYPE_POSITION_DATA);
    WriteFloat(&stream, x);
    WriteFloat(&stream, y);
    WriteFloat(&stream, z);
    enet_peer_send(server->player[playerID].peer, 0, packet);
}

static void receiveGrenadePacket(Server* server, uint8 playerID, DataStream* data)
{
    uint8 ID = ReadByte(data);
    if (playerID != ID) {
        printf("Assigned ID: %d doesnt match sent ID: %d in grenade packet\n", playerID, ID);
    }
    time_t timeNow = get_nanos();
    if (!diffIsOlderThen(timeNow, &server->player[playerID].timers.sinceLastGrenadeThrown, NANO_IN_MILLI * 1000)) {
        return;
    }
    if (server->player[playerID].grenades > 0) {
        for (int i = 0; i < 3; ++i) {
            if (server->player[playerID].grenade[i].sent == 0) {
                server->player[playerID].grenade[i].fuse       = ReadFloat(data);
                server->player[playerID].grenade[i].position.x = ReadFloat(data);
                server->player[playerID].grenade[i].position.y = ReadFloat(data);
                server->player[playerID].grenade[i].position.z = ReadFloat(data);
                float velX = ReadFloat(data), velY = ReadFloat(data), velZ = ReadFloat(data);
                if (server->player[playerID].sprinting) {
                    return;
                }
                float length = sqrt((velX * velX) + (velY * velY) + (velZ * velZ));
                if (length > 2)
                    return;

                float normLength                               = 1 / length;
                server->player[playerID].grenade[i].velocity.x = velX * normLength;
                server->player[playerID].grenade[i].velocity.y = velY * normLength;
                server->player[playerID].grenade[i].velocity.z = velZ * normLength;
                Vector3f posZ1                                 = server->player[playerID].grenade[i].position;
                posZ1.z += 1;
                Vector3f posZ2 = server->player[playerID].grenade[i].position;
                posZ2.z += 2;
                if (vecfValidPos(server->player[playerID].grenade[i].position) ||
                    (vecfValidPos(posZ1) && server->player[playerID].movement.position.z < 0) ||
                    (vecfValidPos(posZ2) && server->player[playerID].movement.position.z < 0))
                {
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
        server->player[playerID].grenades--;
    }
}

// hitPlayerID is the player that got shot
// playerID is the player who fired.
static void receiveHitPacket(Server* server, uint8 playerID, DataStream* data)
{
    uint8    hitPlayerID = ReadByte(data);
    Hit      hitType     = ReadByte(data);
    Vector3f shotPos     = server->player[playerID].movement.position;
    Vector3f shotEyePos  = server->player[playerID].movement.eyePos;
    Vector3f hitPos      = server->player[hitPlayerID].movement.position;
    Vector3f shotOrien   = server->player[playerID].movement.forwardOrientation;
    float    distance    = DistanceIn2D(shotPos, hitPos);
    long     x = 0, y = 0, z = 0;

    if (server->player[playerID].sprinting) {
        return; // Sprinting and hitting somebody is impossible
    }

    time_t timeNow = get_nanos();

    if (allowShot(server, playerID, hitPlayerID, timeNow, distance, &x, &y, &z, shotPos, shotOrien, hitPos, shotEyePos))
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
                    case HIT_TYPE_MELEE:
                        // Empty so we dont have errors :)
                        break;
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
                    case HIT_TYPE_MELEE:
                        // Empty so we dont have errors :)
                        break;
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
                    case HIT_TYPE_MELEE:
                        // Empty so we dont have errors :)
                        break;
                }
                break;
            }
        }
        if (hitType == HIT_TYPE_MELEE) {
            sendHP(server, playerID, hitPlayerID, 80, 1, 2, 5);
        }
    }
}

static void receiveOrientationData(Server* server, uint8 playerID, DataStream* data)
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

static void receiveInputData(Server* server, uint8 playerID, DataStream* data)
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

static void receivePositionData(Server* server, uint8 playerID, DataStream* data)
{
    float x, y, z;
    x = ReadFloat(data);
    y = ReadFloat(data);
    z = ReadFloat(data);
#ifdef DEBUG
    printf("Player: %d, Our X: %f, Y: %f, Z: %f Actual X: %f, Y: %f, Z: %f\n",
           playerID,
           server->player[playerID].movement.position.x,
           server->player[playerID].movement.position.y,
           server->player[playerID].movement.position.z,
           x,
           y,
           z);
    printf("Player: %d, Z solid: %d, Z+1 solid: %d, Z+2 solid: %d, Z: %d, Z+1: %d, Z+2: %d, Crouching: %d\n",
           playerID,
           mapvxlIsSolid(&server->map.map, (int) x, (int) y, (int) z),
           mapvxlIsSolid(&server->map.map, (int) x, (int) y, (int) z + 1),
           mapvxlIsSolid(&server->map.map, (int) x, (int) y, (int) z + 2),
           (int) z,
           (int) z + 1,
           (int) z + 2,
           server->player[playerID].crouching);
#endif
    if (validPlayerPos(server, playerID, x, y, z)) {
        server->player[playerID].movement.position.x   = x;
        server->player[playerID].movement.position.y   = y;
        server->player[playerID].movement.position.z   = z;
        server->player[playerID].movement.prevLegitPos = server->player[playerID].movement.position;
    } else {
        server->player[playerID].movement.position = server->player[playerID].movement.prevLegitPos;
        SendPositionPacket(server,
                           playerID,
                           server->player[playerID].movement.prevLegitPos.x,
                           server->player[playerID].movement.prevLegitPos.y,
                           server->player[playerID].movement.prevLegitPos.z);
    }
}

static void receiveExistingPlayer(Server* server, uint8 playerID, DataStream* data)
{
    StreamSkip(data, 1); // Clients always send a "dumb" ID here since server has not sent them their ID yet

    server->player[playerID].team   = ReadByte(data);
    server->player[playerID].weapon = ReadByte(data);
    server->player[playerID].item   = ReadByte(data);
    server->player[playerID].kills  = ReadInt(data);

    server->protocol.teamUserCount[server->player[playerID].team]++;

    ReadColor3i(data, server->player[playerID].color);
    server->player[playerID].ups = 60;

    uint32 length  = DataLeft(data);
    uint8  invName = 0;
    if (length > 16) {
        WARNING("name too long");
        length = 16;
    } else {
        server->player[playerID].name[length] = '\0';
        ReadArray(data, server->player[playerID].name, length);

        if (strlen(server->player[playerID].name) == 0) {
            snprintf(server->player[playerID].name, strlen("Deuce") + 1, "Deuce");
            length  = 5;
            invName = 1;
        } else if (server->player[playerID].name[0] == '#') {
            snprintf(server->player[playerID].name, strlen("Deuce") + 1, "Deuce");
            length  = 5;
            invName = 1;
        }

        char* lowerCaseName = malloc((strlen(server->player[playerID].name) + 1) * sizeof(char));
        snprintf(lowerCaseName, strlen(server->player[playerID].name), "%s", server->player[playerID].name);

        for (uint32 i = 0; i < strlen(server->player[playerID].name); ++i)
            lowerCaseName[i] = tolower(lowerCaseName[i]);

        char* unwantedNames[] = {"igger", "1gger", "igg3r", "1gg3r", NULL};

        int index = 0;

        while (unwantedNames[index] != NULL) {
            if (strstr(lowerCaseName, unwantedNames[index]) != NULL &&
                strcmp(unwantedNames[index], strstr(lowerCaseName, unwantedNames[index])) == 0)
            {
                snprintf(server->player[playerID].name, strlen("Deuce") + 1, "Deuce");
                length  = 5;
                invName = 1;
                free(lowerCaseName);
                return;
            }
            index++;
        }

        free(lowerCaseName);
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
            snprintf(idChar, 4, "%d", playerID);
            strlcat(server->player[playerID].name, idChar, 17);
        }
    }
    switch (server->player[playerID].weapon) {
        case 0:
            server->player[playerID].weaponReserve  = 50;
            server->player[playerID].weaponClip     = 10;
            server->player[playerID].defaultClip    = RIFLE_DEFAULT_CLIP;
            server->player[playerID].defaultReserve = RIFLE_DEFAULT_RESERVE;
            break;
        case 1:
            server->player[playerID].weaponReserve  = 120;
            server->player[playerID].weaponClip     = 30;
            server->player[playerID].defaultClip    = SMG_DEFAULT_CLIP;
            server->player[playerID].defaultReserve = SMG_DEFAULT_RESERVE;
            break;
        case 2:
            server->player[playerID].weaponReserve  = 48;
            server->player[playerID].weaponClip     = 6;
            server->player[playerID].defaultClip    = SHOTGUN_DEFAULT_CLIP;
            server->player[playerID].defaultReserve = SHOTGUN_DEFAULT_RESERVE;
            break;
    }
    server->player[playerID].state = STATE_SPAWNING;
    if (server->player[playerID].welcomeSent == 0) {
        sendServerNotice(
        server, playerID, "If you find any please contact us on our discord: https://discord.gg/3mqEpQJgY8");
        sendServerNotice(server, playerID, "SpadesX is still in development and thus bugs are expected");
        sendServerNotice(server, playerID, "Welcome to SpadesX server.");
        if (invName) {
            char message[122] =
            "Your name was either empty, had # in front of it or contained something nasty. Your name has been set to ";
            strlcat(message, server->player[playerID].name, 122);
            sendServerNotice(server, playerID, message);
        }
        server->player[playerID].welcomeSent = 1; // So we dont send the message to the player on each time they spawn.
    }

    if (server->protocol.gameMode.intelHeld[0] == 0) {
        SendMoveObject(server, 0, 0, server->protocol.gameMode.intel[0]);
    }
    if (server->protocol.gameMode.intelHeld[1] == 0) {
        SendMoveObject(server, 1, 1, server->protocol.gameMode.intel[1]);
    }
}

static void receiveBlockAction(Server* server, uint8 playerID, DataStream* data)
{
    uint8 ID = ReadByte(data);
    if (playerID != ID) {
        printf("Assigned ID: %d doesnt match sent ID: %d in block action packet\n", playerID, ID);
    }
    if (server->player[playerID].canBuild && server->globalAB) {
        uint8 actionType = ReadByte(data);
        int   X          = ReadInt(data);
        int   Y          = ReadInt(data);
        int   Z          = ReadInt(data);
        if (server->player[playerID].sprinting) {
            return;
        }
        Vector3f vectorBlock  = {X, Y, Z};
        Vector3f playerVector = server->player[playerID].movement.position;
        if (((server->player[playerID].item == 0 && (actionType == 1 || actionType == 2)) ||
             (server->player[playerID].item == 1 && actionType == 0) ||
             (server->player[playerID].item == 2 && actionType == 1)))
        {
            if ((DistanceIn3D(vectorBlock, playerVector) <= 4 || server->player[playerID].item == 2) &&
                vecfValidPos(vectorBlock)) {
                switch (actionType) {
                    case 0:
                    {
                        if (gamemodeBlockChecks(server, X, Y, Z)) {
                            time_t timeNow = get_nanos();
                            if (server->player[playerID].blocks > 0 &&
                                diffIsOlderThen(
                                timeNow, &server->player[playerID].timers.sinceLastBlockPlac, NANO_IN_MILLI * 100))
                            {
                                mapvxlSetColor(&server->map.map, X, Y, Z, server->player[playerID].toolColor.color);
                                server->player[playerID].blocks--;
                                moveIntelAndTentUp(server);
                                SendBlockAction(server, playerID, actionType, X, Y, Z);
                            }
                        }
                    } break;

                    case 1:
                    {
                        if (Z < 62 && gamemodeBlockChecks(server, X, Y, Z)) {
                            time_t timeNow = get_nanos();
                            if (diffIsOlderThen(
                                timeNow, &server->player[playerID].timers.sinceLastBlockDest, NANO_IN_MILLI * 100)) {
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
                                SendBlockAction(server, playerID, actionType, X, Y, Z);
                            }
                        }
                    } break;

                    case 2:
                    {
                        if (gamemodeBlockChecks(server, X, Y, Z) && gamemodeBlockChecks(server, X, Y, Z + 1) &&
                            gamemodeBlockChecks(server, X, Y, Z - 1))
                        {
                            time_t timeNow = get_nanos();
                            if (diffIsOlderThen(
                                timeNow, &server->player[playerID].timers.sinceLast3BlockDest, NANO_IN_MILLI * 300)) {
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
                                SendBlockAction(server, playerID, actionType, X, Y, Z);
                            }
                        }
                    } break;
                }
                moveIntelAndTentDown(server);
            }
        } else {
            printf("Player: #%d may be using BlockExploit with Item: %d and Action: %d\n",
                   playerID,
                   server->player[playerID].item,
                   actionType);
        }
    }
}

static void receiveBlockLine(Server* server, uint8 playerID, DataStream* data)
{
    uint8 ID = ReadByte(data);
    if (playerID != ID) {
        printf("Assigned ID: %d doesnt match sent ID: %d in blockline packet\n", playerID, ID);
    }
    if (server->player[playerID].blocks > 0 && server->player[playerID].canBuild && server->globalAB &&
        server->player[playerID].item == 1 &&
        diffIsOlderThen(get_nanos(), &server->player[playerID].timers.sinceLastBlockPlac, NANO_IN_MILLI * 100))
    {
        Vector3i start;
        Vector3i end;
        start.x = ReadInt(data);
        start.y = ReadInt(data);
        start.z = ReadInt(data);
        end.x   = ReadInt(data);
        end.y   = ReadInt(data);
        end.z   = ReadInt(data);
        if (server->player[playerID].sprinting) {
            return;
        }
        Vector3f startF = {start.x, start.y, start.z};
        Vector3f endF   = {end.x, end.y, end.z};
        if (DistanceIn3D(endF, server->player[playerID].movement.position) <= 4 &&
            DistanceIn3D(startF, server->player[playerID].locAtClick) <= 4 && vecfValidPos(startF) &&
            vecfValidPos(endF))
        {
            int size = blockLine(&start, &end, server->map.resultLine);
            server->player[playerID].blocks -= size;
            for (int i = 0; i < size; i++) {
                mapvxlSetColor(&server->map.map,
                               server->map.resultLine[i].x,
                               server->map.resultLine[i].y,
                               server->map.resultLine[i].z,
                               server->player[playerID].toolColor.color);
            }
            moveIntelAndTentUp(server);
            SendBlockLine(server, playerID, start, end);
        }
    }
}

static void receiveSetTool(Server* server, uint8 playerID, DataStream* data)
{
    uint8 ID   = ReadByte(data);
    uint8 tool = ReadByte(data);
    if (server->player[playerID].item == tool) {
        return;
    }
    if (playerID != ID) {
        printf("Assigned ID: %d doesnt match sent ID: %d in set tool packet\n", playerID, ID);
    }
    server->player[playerID].item = tool;
    SendSetTool(server, playerID, tool);
}

static void receiveSetColor(Server* server, uint8 playerID, DataStream* data)
{
    uint8 ID = ReadByte(data);
    uint8 B  = ReadByte(data);
    uint8 G  = ReadByte(data);
    uint8 R  = ReadByte(data);
    uint8 A  = 0;

    if (playerID != ID) {
        printf("Assigned ID: %d doesnt match sent ID: %d in set color packet\n", playerID, ID);
    }

    server->player[playerID].toolColor.colorArray[A_CHANNEL] = A;
    server->player[playerID].toolColor.colorArray[R_CHANNEL] = R;
    server->player[playerID].toolColor.colorArray[G_CHANNEL] = G;
    server->player[playerID].toolColor.colorArray[B_CHANNEL] = B;
    SendSetColor(server, playerID, R, G, B);
}

static void receiveWeaponInput(Server* server, uint8 playerID, DataStream* data)
{
    uint8 mask   = 1;
    uint8 ID     = ReadByte(data);
    uint8 wInput = ReadByte(data);
    if (playerID != ID) {
        printf("Assigned ID: %d doesnt match sent ID: %d in weapon input packet\n", playerID, ID);
    } else if (server->player[playerID].state != STATE_READY) {
        return;
    } else if (server->player[playerID].sprinting) {
        wInput = 0; /* Do not return just set it to 0 as we want to send to players that the player is no longer
                    shooting when they start sprinting */
    }
    server->player[playerID].primary_fire   = wInput & mask;
    server->player[playerID].secondary_fire = (wInput >> 1) & mask;

    if (server->player[playerID].secondary_fire && server->player[playerID].item == 1) {
        server->player[playerID].locAtClick = server->player[playerID].movement.position;
    }

    else if (server->player[playerID].weaponClip > 0)
    {
        SendWeaponInput(server, playerID, wInput);

        if (server->player[playerID].primary_fire) {
            server->player[playerID].timers.sinceLastWeaponInput = get_nanos();
            server->player[playerID].weaponClip--;
            if ((server->player[playerID].movement.previousOrientation.x ==
                 server->player[playerID].movement.forwardOrientation.x) &&
                (server->player[playerID].movement.previousOrientation.y ==
                 server->player[playerID].movement.forwardOrientation.y) &&
                (server->player[playerID].movement.previousOrientation.z ==
                 server->player[playerID].movement.forwardOrientation.z) &&
                server->player[playerID].item == TOOL_GUN)
            {
                for (int i = 0; i < server->protocol.maxPlayers; ++i) {
                    if (isPastJoinScreen(server, i) && isStaff(server, i)) {
                        char message[200];
                        snprintf(message, 200, "WARNING. Player %d may be using no recoil", playerID);
                        sendServerNotice(server, i, message);
                    }
                }
            }
            server->player[playerID].movement.previousOrientation =
            server->player[playerID].movement.forwardOrientation;
        }
    } else {
        // sendKillPacket(server, playerID, playerID, 0, 30, 0);
    }
}

static void receiveWeaponReload(Server* server, uint8 playerID, DataStream* data)
{
    uint8 ID      = ReadByte(data);
    uint8 clip    = ReadByte(data);
    uint8 reserve = ReadByte(data);
    if (playerID != ID) {
        printf("Assigned ID: %d doesnt match sent ID: %d in weapon reload packet\n", playerID, ID);
    }
    server->player[playerID].primary_fire   = 0;
    server->player[playerID].secondary_fire = 0;

    uint8 toRefill = server->player[playerID].defaultClip - server->player[playerID].weaponClip;
    if (server->player[playerID].weaponReserve < toRefill) {
        if (server->player[playerID].weaponReserve == 0) {
            return;
        }
        server->player[playerID].weaponClip += server->player[playerID].weaponReserve;
        server->player[playerID].weaponReserve = 0;
        SendWeaponReload(server, playerID);
        return;
    }
    server->player[playerID].weaponClip += toRefill;
    server->player[playerID].weaponReserve -= toRefill;
    if (server->player[playerID].weaponClip != clip || server->player[playerID].weaponReserve != reserve) {
        // Do nothing for now. Voxlap is DUMB AF and sends only 0 0 and OpenSpades is sorta dumb as well and sends 255
        // 255. How am i supposed to check for hacks dammit with this kind of info dammit ?
    }
    SendWeaponReload(server, playerID);
}

static void receiveChangeTeam(Server* server, uint8 playerID, DataStream* data)
{
    uint8 ID = ReadByte(data);
    server->protocol.teamUserCount[server->player[playerID].team]--;
    uint8 team                    = ReadByte(data);
    server->player[playerID].team = team;
    if (playerID != ID) {
        printf("Assigned ID: %d doesnt match sent ID: %d in change team packet\n", playerID, ID);
    }
    server->protocol.teamUserCount[server->player[playerID].team]++;
    sendKillPacket(server, playerID, playerID, 5, 5, 0);
    server->player[playerID].state = STATE_WAITING_FOR_RESPAWN;
}

static void receiveChangeWeapon(Server* server, uint8 playerID, DataStream* data)
{
    uint8 ID     = ReadByte(data);
    uint8 weapon = ReadByte(data);
    if (server->player[playerID].weapon == weapon) {
        return;
    }
    server->player[playerID].weapon = weapon;
    if (playerID != ID) {
        printf("Assigned ID: %d doesnt match sent ID: %d in change weapon packet\n", playerID, ID);
    }
    sendKillPacket(server, playerID, playerID, 6, 5, 0);
    server->player[playerID].state = STATE_WAITING_FOR_RESPAWN;
}

static void receiveVersionResponse(Server* server, uint8 playerID, DataStream* data)
{
    server->player[playerID].client           = ReadByte(data);
    server->player[playerID].version_major    = ReadByte(data);
    server->player[playerID].version_minor    = ReadByte(data);
    server->player[playerID].version_revision = ReadByte(data);
    uint32 length                             = DataLeft(data);
    if (length < 256) {
        server->player[playerID].os_info[length] = '\0';
        ReadArray(data, server->player[playerID].os_info, length);
    } else {
        snprintf(server->player[playerID].os_info, 8, "Unknown");
    }
    if (server->player[playerID].client == 'o') {
        if (!(server->player[playerID].version_major == 0 && server->player[playerID].version_minor == 1 &&
              (server->player[playerID].version_revision == 3 || server->player[playerID].version_revision == 5)) &&
            strstr(server->player[playerID].os_info, "ZeroSpades") == NULL)
        {
            enet_peer_disconnect(server->player[playerID].peer, REASON_KICKED);
        }
    } else if (server->player[playerID].client == 'B') {
        if (!(server->player[playerID].version_major == 0 && server->player[playerID].version_minor == 1 &&
              server->player[playerID].version_revision == 5))
        {
            enet_peer_disconnect(server->player[playerID].peer, REASON_KICKED);
        }
    }
}
void OnPacketReceived(Server* server, uint8 playerID, DataStream* data, ENetEvent event)
{
    PacketID type = (PacketID) ReadByte(data);
    switch (type) {
        case PACKET_TYPE_EXISTING_PLAYER:
            receiveExistingPlayer(server, playerID, data);
            break;
        case PACKET_TYPE_POSITION_DATA:
            receivePositionData(server, playerID, data);
            break;
        case PACKET_TYPE_ORIENTATION_DATA:
            receiveOrientationData(server, playerID, data);
            break;
        case PACKET_TYPE_INPUT_DATA:
            receiveInputData(server, playerID, data);
            break;
        case PACKET_TYPE_CHAT_MESSAGE:
            handleAndSendMessage(event, data, server, playerID);
            break;
        case PACKET_TYPE_BLOCK_ACTION:
            receiveBlockAction(server, playerID, data);
            break;
        case PACKET_TYPE_BLOCK_LINE:
            receiveBlockLine(server, playerID, data);
            break;
        case PACKET_TYPE_SET_TOOL:
            receiveSetTool(server, playerID, data);
            break;
        case PACKET_TYPE_SET_COLOR:
            receiveSetColor(server, playerID, data);
            break;
        case PACKET_TYPE_WEAPON_INPUT:
            receiveWeaponInput(server, playerID, data);
            break;
        case PACKET_TYPE_HIT_PACKET:
            receiveHitPacket(server, playerID, data);
            break;
        case PACKET_TYPE_WEAPON_RELOAD:
            receiveWeaponReload(server, playerID, data);
            break;
        case PACKET_TYPE_CHANGE_TEAM:
            receiveChangeTeam(server, playerID, data);
            break;
        case PACKET_TYPE_CHANGE_WEAPON:
            receiveChangeWeapon(server, playerID, data);
            break;
        case PACKET_TYPE_GRENADE_PACKET:
            receiveGrenadePacket(server, playerID, data);
            break;
        case PACKET_TYPE_VERSION_RESPONSE:
            receiveVersionResponse(server, playerID, data);
            break;
        default:
            printf("unhandled input, id %u, code %u\n", playerID, type);
            break;
    }
}
