// Copyright DarkNeutrino 2021
#include "../Extern/libmapvxl/libmapvxl.h"
#include "Commands.h"
#include "ParseConvert.h"
#include "Protocol.h"
#include "Structs.h"
#include "Util/Compress.h"
#include "Util/DataStream.h"
#include "Util/Enums.h"
#include "Util/Line.h"
#include "Util/Physics.h"
#include "Util/Queue.h"
#include "Util/Types.h"
#include "Util/Utlist.h"

#include <ctype.h>
#include <enet/enet.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#ifdef _WIN32
    #define strlcat(dst, src, siz) strcat_s(dst, siz, src)
#else
    #include <bsd/string.h>
#endif

inline static uint8 allowShot(Server*  server,
                              uint8    playerID,
                              uint8    hitPlayerID,
                              uint64   timeNow,
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
        (server->player[playerID].allowKilling && server->globalAK) && validateHit(shotPos, shotOrien, hitPos, 5) &&
        (castRay(
         server, shotEyePos.x, shotEyePos.y, shotEyePos.z, shotOrien.x, shotOrien.y, shotOrien.z, distance, x, y, z) ==
         0 ||
         castRay(server,
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

void sendRestock(Server* server, uint8 playerID)
{
    if (server->protocol.numPlayers == 0) {
        return;
    }
    ENetPacket* packet = enet_packet_create(NULL, 2, ENET_PACKET_FLAG_RELIABLE);
    DataStream  stream = {packet->data, packet->dataLength, 0};
    WriteByte(&stream, PACKET_TYPE_RESTOCK);
    WriteByte(&stream, playerID);
    if (enet_peer_send(server->player[playerID].peer, 0, packet) != 0) {
        enet_packet_destroy(packet);
    }
}

void sendMoveObject(Server* server, uint8 object, uint8 team, Vector3f pos)
{
    if (server->protocol.numPlayers == 0) {
        return;
    }
    ENetPacket* packet = enet_packet_create(NULL, 15, ENET_PACKET_FLAG_RELIABLE);
    DataStream  stream = {packet->data, packet->dataLength, 0};
    WriteByte(&stream, PACKET_TYPE_MOVE_OBJECT);
    WriteByte(&stream, object);
    WriteByte(&stream, team);
    WriteFloat(&stream, pos.x);
    WriteFloat(&stream, pos.y);
    WriteFloat(&stream, pos.z);

    uint8 sent = 0;
    for (int player = 0; player < server->protocol.maxPlayers; ++player) {
        if (isPastStateData(server, player)) {
            if (enet_peer_send(server->player[player].peer, 0, packet) == 0) {
                sent = 1;
            }
        }
    }
    if (sent == 0) {
        enet_packet_destroy(packet);
    }
}

void sendIntelCapture(Server* server, uint8 playerID, uint8 winning)
{
    if (server->protocol.numPlayers == 0) {
        return;
    }
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

    uint8 sent = 0;
    for (int player = 0; player < server->protocol.maxPlayers; ++player) {
        if (isPastStateData(server, player)) {
            if (enet_peer_send(server->player[player].peer, 0, packet) == 0) {
                sent = 1;
            }
        }
    }
    if (sent == 0) {
        enet_packet_destroy(packet);
    }
}

void sendIntelPickup(Server* server, uint8 playerID)
{
    if (server->protocol.numPlayers == 0) {
        return;
    }
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

    uint8 sent = 0;
    for (int player = 0; player < server->protocol.maxPlayers; ++player) {
        if (isPastStateData(server, player)) {
            if (enet_peer_send(server->player[player].peer, 0, packet) == 0) {
                sent = 1;
            }
        }
    }
    if (sent == 0) {
        enet_packet_destroy(packet);
    }
}

void sendIntelDrop(Server* server, uint8 playerID)
{
    if (server->protocol.numPlayers == 0) {
        return;
    }
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
        WriteFloat(&stream, (float) server->map.map.MAP_X_MAX / 2);
        WriteFloat(&stream, (float) server->map.map.MAP_Y_MAX / 2);
        WriteFloat(
        &stream,
        (float) mapvxlFindTopBlock(&server->map.map, server->map.map.MAP_X_MAX / 2, server->map.map.MAP_Y_MAX / 2));

        server->protocol.gameMode.intel[team].x = (float) server->map.map.MAP_X_MAX / 2;
        server->protocol.gameMode.intel[team].y = (float) server->map.map.MAP_Y_MAX / 2;
        server->protocol.gameMode.intel[team].z =
        mapvxlFindTopBlock(&server->map.map, server->map.map.MAP_X_MAX / 2, server->map.map.MAP_Y_MAX / 2);
        server->protocol.gameMode.intel[server->player[playerID].team] = server->protocol.gameMode.intel[team];
        sendMoveObject(
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

    LOG_INFO("Dropping intel at X: %d, Y: %d, Z: %d",
             (int) server->protocol.gameMode.intel[team].x,
             (int) server->protocol.gameMode.intel[team].y,
             (int) server->protocol.gameMode.intel[team].z);
    uint8 sent = 0;
    for (int player = 0; player < server->protocol.maxPlayers; ++player) {
        if (isPastStateData(server, player)) {
            if (enet_peer_send(server->player[player].peer, 0, packet) == 0) {
                sent = 1;
            }
        }
    }
    if (sent == 0) {
        enet_packet_destroy(packet);
    }
}

void sendGrenade(Server* server, uint8 playerID, float fuse, Vector3f position, Vector3f velocity)
{
    if (server->protocol.numPlayers == 0) {
        return;
    }
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

void sendPlayerLeft(Server* server, uint8 playerID)
{
    char ipString[17];
    formatIPToString(ipString, server->player[playerID].ipStruct);
    LOG_INFO("Player %s (%s, #%hhu) disconnected", server->player[playerID].name, ipString, playerID);
    if (server->protocol.numPlayers == 0) {
        return;
    }
    for (uint8 i = 0; i < server->protocol.maxPlayers; ++i) {
        if (i != playerID && isPastStateData(server, i)) {
            ENetPacket* packet = enet_packet_create(NULL, 2, ENET_PACKET_FLAG_RELIABLE);
            DataStream  stream = {packet->data, packet->dataLength, 0};
            WriteByte(&stream, PACKET_TYPE_PLAYER_LEFT);
            WriteByte(&stream, playerID);

            if (enet_peer_send(server->player[i].peer, 0, packet) != 0) {
                LOG_WARNING("Failed to send player left event");
                enet_packet_destroy(packet);
            }
        }
    }
}

void sendWeaponReload(Server* server, uint8 playerID, uint8 startAnimation, uint8 clip, uint8 reserve)
{
    if (server->protocol.numPlayers == 0) {
        return;
    }
    ENetPacket* packet = enet_packet_create(NULL, 4, ENET_PACKET_FLAG_RELIABLE);
    DataStream  stream = {packet->data, packet->dataLength, 0};
    WriteByte(&stream, PACKET_TYPE_WEAPON_RELOAD);
    WriteByte(&stream, playerID);
    if (startAnimation) {
        WriteByte(&stream, clip);
        WriteByte(&stream, reserve);
    } else {
        WriteByte(&stream, server->player[playerID].weaponClip);
        WriteByte(&stream, server->player[playerID].weaponReserve);
    }
    if (startAnimation) {
        uint8 sendSucc = 0;
        for (int player = 0; player < server->protocol.maxPlayers; ++player) {
            if (isPastStateData(server, player) && player != playerID) {
                if (enet_peer_send(server->player[player].peer, 0, packet) == 0) {
                    sendSucc = 1;
                }
            }
        }
        if (sendSucc == 0) {
            enet_packet_destroy(packet);
        }
    } else {
        if (isPastJoinScreen(server, playerID)) {
            if (enet_peer_send(server->player[playerID].peer, 0, packet) != 0) {
                enet_packet_destroy(packet);
            }
        }
    }
}

void sendWeaponInput(Server* server, uint8 playerID, uint8 wInput)
{
    if (server->protocol.numPlayers == 0) {
        return;
    }
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

void sendSetColor(Server* server, uint8 playerID, uint8 R, uint8 G, uint8 B)
{
    if (server->protocol.numPlayers == 0) {
        return;
    }
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

void sendSetTool(Server* server, uint8 playerID, uint8 tool)
{
    if (server->protocol.numPlayers == 0) {
        return;
    }
    ENetPacket* packet = enet_packet_create(NULL, 3, ENET_PACKET_FLAG_RELIABLE);
    DataStream  stream = {packet->data, packet->dataLength, 0};
    WriteByte(&stream, PACKET_TYPE_SET_TOOL);
    WriteByte(&stream, playerID);
    WriteByte(&stream, tool);
    if (SendPacketExceptSender(server, packet, playerID) == 0) {
        enet_packet_destroy(packet);
    }
}

void sendBlockLine(Server* server, uint8 playerID, Vector3i start, Vector3i end)
{
    if (server->protocol.numPlayers == 0) {
        return;
    }
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
    uint8 sent = 0;
    for (int player = 0; player < server->protocol.maxPlayers; ++player) {
        if (isPastStateData(server, player)) {
            if (enet_peer_send(server->player[player].peer, 0, packet) == 0) {
                sent = 1;
            }
        }
    }
    if (sent == 0) {
        enet_packet_destroy(packet);
    }
}

void sendBlockAction(Server* server, uint8 playerID, uint8 actionType, int X, int Y, int Z)
{
    if (server->protocol.numPlayers == 0) {
        return;
    }
    ENetPacket* packet = enet_packet_create(NULL, 15, ENET_PACKET_FLAG_RELIABLE);
    DataStream  stream = {packet->data, packet->dataLength, 0};
    WriteByte(&stream, PACKET_TYPE_BLOCK_ACTION);
    WriteByte(&stream, playerID);
    WriteByte(&stream, actionType);
    WriteInt(&stream, X);
    WriteInt(&stream, Y);
    WriteInt(&stream, Z);
    uint8 sent = 0;
    for (int player = 0; player < server->protocol.maxPlayers; ++player) {
        if (isPastStateData(server, player)) {
            if (enet_peer_send(server->player[player].peer, 0, packet) == 0) {
                sent = 1;
            }
        }
    }
    if (sent == 0) {
        enet_packet_destroy(packet);
    }
}

void sendStateData(Server* server, uint8 playerID)
{
    if (server->protocol.numPlayers == 0) {
        return;
    }
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
    } else {
        enet_packet_destroy(packet);
    }
}

void sendInputData(Server* server, uint8 playerID)
{
    if (server->protocol.numPlayers == 0) {
        return;
    }
    ENetPacket* packet = enet_packet_create(NULL, 3, ENET_PACKET_FLAG_RELIABLE);
    DataStream  stream = {packet->data, packet->dataLength, 0};
    WriteByte(&stream, PACKET_TYPE_INPUT_DATA);
    WriteByte(&stream, playerID);
    WriteByte(&stream, server->player[playerID].input);
    if (SendPacketExceptSenderDistCheck(server, packet, playerID) == 0) {
        enet_packet_destroy(packet);
    }
}

void sendKillActionPacket(Server* server,
                          uint8   killerID,
                          uint8   playerID,
                          uint8   killReason,
                          uint8   respawnTime,
                          uint8   makeInvisible)
{
    if (server->protocol.numPlayers == 0) {
        return;
    }
    ENetPacket* packet = enet_packet_create(NULL, 5, ENET_PACKET_FLAG_RELIABLE);
    DataStream  stream = {packet->data, packet->dataLength, 0};
    WriteByte(&stream, PACKET_TYPE_KILL_ACTION);
    WriteByte(&stream, playerID);    // Player that died.
    WriteByte(&stream, killerID);    // Player that killed.
    WriteByte(&stream, killReason);  // Killing reason (1 is headshot)
    WriteByte(&stream, respawnTime); // Time before respawn happens
    uint8 sent = 0;
    for (int player = 0; player < server->protocol.maxPlayers; ++player) {
        uint8 isPast = isPastStateData(server, player);
        if ((makeInvisible && player != playerID && isPast) || (isPast && !makeInvisible)) {
            if (enet_peer_send(server->player[player].peer, 0, packet) == 0) {
                sent = 1;
            }
        }
    }
    if (sent == 0) {
        enet_packet_destroy(packet);
        return; // Do not kill the player since sending the packet failed
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
        sendIntelDrop(server, playerID);
    }
}

void sendSetHP(Server*  server,
               uint8    playerID,
               uint8    hitPlayerID,
               long     HPChange,
               uint8    typeOfDamage,
               uint8    killReason,
               uint8    respawnTime,
               uint8    isGrenade,
               Vector3f position)
{
    if (server->protocol.numPlayers == 0 || server->player[hitPlayerID].team == TEAM_SPECTATOR ||
        (!server->player[playerID].allowTeamKilling &&
         server->player[playerID].team == server->player[hitPlayerID].team && playerID != hitPlayerID))
    {
        return;
    }
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
            sendKillActionPacket(server, playerID, hitPlayerID, killReason, respawnTime, 0);
            return;
        }

        else if (server->player[hitPlayerID].HP > 0 && server->player[hitPlayerID].HP < 100)
        {
            ENetPacket* packet = enet_packet_create(NULL, 15, ENET_PACKET_FLAG_RELIABLE);
            DataStream  stream = {packet->data, packet->dataLength, 0};
            WriteByte(&stream, PACKET_TYPE_SET_HP);
            WriteByte(&stream, server->player[hitPlayerID].HP);
            WriteByte(&stream, typeOfDamage);
            if (typeOfDamage != 0 && isGrenade == 0) {
                WriteFloat(&stream, server->player[playerID].movement.position.x);
                WriteFloat(&stream, server->player[playerID].movement.position.y);
                WriteFloat(&stream, server->player[playerID].movement.position.z);
            } else if (typeOfDamage != 0 && isGrenade == 1) {
                WriteFloat(&stream, position.x);
                WriteFloat(&stream, position.y);
                WriteFloat(&stream, position.z);
            } else {
                WriteFloat(&stream, 0);
                WriteFloat(&stream, 0);
                WriteFloat(&stream, 0);
            }
            if (enet_peer_send(server->player[hitPlayerID].peer, 0, packet) != 0) {
                enet_packet_destroy(packet);
            }
        }
    }
}

void sendExistingPlayer(Server* server, uint8 playerID, uint8 otherID)
{
    if (server->protocol.numPlayers == 0) {
        return;
    }
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
        LOG_WARNING("Failed to send player state");
        enet_packet_destroy(packet);
    }
}

void sendVersionRequest(Server* server, uint8 playerID)
{
    if (server->protocol.numPlayers == 0) {
        return;
    }
    ENetPacket* packet = enet_packet_create(NULL, 1, ENET_PACKET_FLAG_RELIABLE);
    DataStream  stream = {packet->data, packet->dataLength, 0};
    WriteByte(&stream, PACKET_TYPE_VERSION_REQUEST);
    if (enet_peer_send(server->player[playerID].peer, 0, packet) != 0) {
        enet_packet_destroy(packet);
    }
}
void sendMapStart(Server* server, uint8 playerID)
{
    if (server->protocol.numPlayers == 0) {
        return;
    }
    LOG_INFO("Sending map info to %s (#%hhu)", server->player[playerID].name, playerID);
    ENetPacket* packet = enet_packet_create(NULL, 5, ENET_PACKET_FLAG_RELIABLE);
    DataStream  stream = {packet->data, packet->dataLength, 0};
    WriteByte(&stream, PACKET_TYPE_MAP_START);
    WriteInt(&stream, server->map.compressedSize);
    if (enet_peer_send(server->player[playerID].peer, 0, packet) == 0) {
        server->player[playerID].state = STATE_LOADING_CHUNKS;

        // The biggest possible VXL size given the XYZ size
        uint8* map = (uint8*) calloc(
        server->map.map.MAP_X_MAX * server->map.map.MAP_Y_MAX * (server->map.map.MAP_Z_MAX / 2), sizeof(uint8));
        // Write map to out
        server->map.mapSize = mapvxlWriteMap(&server->map.map, map);
        // Resize the map to the exact VXL memory size for given XYZ coordinate size
        uint8* old_map;
        old_map = (uint8*) realloc(map, server->map.mapSize);
        if (!old_map) {
            free(map);
            return;
        }
        map                             = old_map;
        server->map.compressedMap       = CompressData(map, server->map.mapSize, DEFAULT_COMPRESSOR_CHUNK_SIZE);
        server->player[playerID].queues = server->map.compressedMap;
        free(map);
    }
}

void sendMapChunks(Server* server, uint8 playerID)
{
    if (server->player[playerID].queues == NULL) {
        while (server->map.compressedMap) {
            server->map.compressedMap = Pop(server->map.compressedMap);
        }
        sendVersionRequest(server, playerID);
        server->player[playerID].state = STATE_JOINING;
        LOG_INFO("Finished sending map to %s (#%hhu)", server->player[playerID].name, playerID);
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

void sendCreatePlayer(Server* server, uint8 playerID, uint8 otherID)
{
    if (server->protocol.numPlayers == 0) {
        return;
    }
    ENetPacket* packet = enet_packet_create(NULL, 32, ENET_PACKET_FLAG_RELIABLE);
    DataStream  stream = {packet->data, packet->dataLength, 0};
    WriteByte(&stream, PACKET_TYPE_CREATE_PLAYER);
    WriteByte(&stream, otherID);                                       // ID
    WriteByte(&stream, server->player[otherID].weapon);                // WEAPON
    WriteByte(&stream, server->player[otherID].team);                  // TEAM
    WriteVector3f(&stream, server->player[otherID].movement.position); // X Y Z
    WriteArray(&stream, server->player[otherID].name, 16);             // NAME

    if (enet_peer_send(server->player[playerID].peer, 0, packet) != 0) {
        LOG_WARNING("Failed to send player state");
        enet_packet_destroy(packet);
    }
}

void SendRespawn(Server* server, uint8 playerID)
{
    for (uint8 i = 0; i < server->protocol.maxPlayers; ++i) {
        if (isPastJoinScreen(server, i)) {
            sendCreatePlayer(server, i, playerID);
        }
    }
    server->player[playerID].state = STATE_READY;
}

void handleAndSendMessage(ENetEvent event, DataStream* data, Server* server, uint8 player)
{
    if (server->protocol.numPlayers == 0) {
        return;
    }
    uint32 packetSize = event.packet->dataLength + 1;
    uint8  ID         = ReadByte(data);
    int    meantfor   = ReadByte(data);
    uint32 length     = DataLeft(data);
    char*  message    = calloc(length + 1, sizeof(char));
    ReadArray(data, message, length);
    if (player != ID) {
        LOG_WARNING("Assigned ID: %d doesnt match sent ID: %d in message packet", player, ID);
    }
    uint8 resetTime = 1;
    if (!diffIsOlderThenDontUpdate(
        getNanos(), server->player[player].timers.sinceLastMessageForSpam, (uint64) NANO_IN_MILLI * 2000) &&
        !server->player[player].muted && server->player[player].permLevel <= 1)
    {
        server->player[player].spamCounter++;
        if (server->player[player].spamCounter >= 5) {
            sendMessageToStaff(
            server, "WARNING: Player %s (#%d) is trying to spam. Muting.", server->player[player].name, player);
            sendServerNotice(server,
                             player,
                             0,
                             "SERVER: You have been muted for excessive spam. If you feel like this is a mistake "
                             "contact staff via /admin command");
            server->player[player].muted       = 1;
            server->player[player].spamCounter = 0;
        }
        resetTime = 0;
    }
    if (resetTime) {
        server->player[player].timers.sinceLastMessageForSpam = getNanos();
        server->player[player].spamCounter                    = 0;
    }

    if (!diffIsOlderThen(getNanos(), &server->player[player].timers.sinceLastMessage, (uint64) NANO_IN_MILLI * 400) &&
        server->player[player].permLevel <= 1)
    {
        sendServerNotice(
        server, player, 0, "WARNING: You sent last message too fast and thus was not sent out to players");
        return;
    }

    message[length] = '\0';
    char meantFor[7];
    switch (meantfor) {
        case 0:
            snprintf(meantFor, 7, "Global");
            break;
        case 1:
            snprintf(meantFor, 5, "Team");
            break;
        case 2:
            snprintf(meantFor, 7, "System");
            break;
    }
    LOG_INFO("Player %s (#%hhu) (%s) said: %s", server->player[player].name, player, meantFor, message);

    uint8 sent = 0;
    if (message[0] == '/') {
        handleCommands(server, player, message, 0);
    } else {
        if (!server->player[player].muted) {
            ENetPacket* packet = enet_packet_create(NULL, packetSize, ENET_PACKET_FLAG_RELIABLE);
            DataStream  stream = {packet->data, packet->dataLength, 0};
            WriteByte(&stream, PACKET_TYPE_CHAT_MESSAGE);
            WriteByte(&stream, player);
            WriteByte(&stream, meantfor);
            WriteArray(&stream, message, length);
            for (int playerID = 0; playerID < server->protocol.maxPlayers; ++playerID) {
                if (isPastJoinScreen(server, playerID) && !server->player[player].muted &&
                    ((server->player[playerID].team == server->player[player].team && meantfor == 1) || meantfor == 0))
                {
                    if (enet_peer_send(server->player[playerID].peer, 0, packet) == 0) {
                        sent = 1;
                    }
                }
            }
            if (sent == 0) {
                enet_packet_destroy(packet);
            }
        }
    }
    free(message);
}

void SendWorldUpdate(Server* server, uint8 playerID)
{
    if (server->protocol.numPlayers == 0) {
        return;
    }
    ENetPacket* packet = enet_packet_create(NULL, 1 + (32 * 24), 0);
    DataStream  stream = {packet->data, packet->dataLength, 0};
    WriteByte(&stream, PACKET_TYPE_WORLD_UPDATE);

    for (uint8 j = 0; j < server->protocol.maxPlayers; ++j) {
        if (playerToPlayerVisible(server, playerID, j) && server->player[j].isInvisible == 0) {
            /*float    dt       = (getNanos() - server->globalTimers.lastUpdateTime) / 1000000000.f;
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
    if (enet_peer_send(server->player[playerID].peer, 0, packet) != 0) {
        enet_packet_destroy(packet);
    }
}

void SendPositionPacket(Server* server, uint8 playerID, float x, float y, float z)
{
    if (server->protocol.numPlayers == 0) {
        return;
    }
    ENetPacket* packet = enet_packet_create(NULL, 13, ENET_PACKET_FLAG_RELIABLE);
    DataStream  stream = {packet->data, packet->dataLength, 0};
    WriteByte(&stream, PACKET_TYPE_POSITION_DATA);
    WriteFloat(&stream, x);
    WriteFloat(&stream, y);
    WriteFloat(&stream, z);
    if (enet_peer_send(server->player[playerID].peer, 0, packet) != 0) {
        enet_packet_destroy(packet);
    }
}

static void receiveGrenadePacket(Server* server, uint8 playerID, DataStream* data)
{
    uint8 ID = ReadByte(data);
    if (playerID != ID) {
        LOG_WARNING("Assigned ID: %d doesnt match sent ID: %d in grenade packet", playerID, ID);
    }
    if (server->player[playerID].primary_fire && server->player[playerID].item == 1) {
        sendServerNotice(server, playerID, 0, "InstaSuicideNade detected. Grenade ineffective");
        sendMessageToStaff(
        server, 0, "Player %s (#%hhu) tried to use InstaSpadeNade", server->player[playerID].name, playerID);
        return;
    }
    uint64 timeNow = getNanos();
    if (!diffIsOlderThen(timeNow, &server->player[playerID].timers.sinceLastGrenadeThrown, NANO_IN_MILLI * 500) ||
        !diffIsOlderThen(timeNow, &server->player[playerID].timers.sincePossibleSpadenade, (long) NANO_IN_MILLI * 1000))
    {
        return;
    }
    Grenade* grenade = malloc(sizeof(Grenade));
    if (server->player[playerID].grenades > 0) {
        grenade->fuse       = ReadFloat(data);
        grenade->position.x = ReadFloat(data);
        grenade->position.y = ReadFloat(data);
        grenade->position.z = ReadFloat(data);
        float velX = ReadFloat(data), velY = ReadFloat(data), velZ = ReadFloat(data);
        if (server->player[playerID].sprinting) {
            free(grenade);
            return;
        }
        float length = sqrt((velX * velX) + (velY * velY) + (velZ * velZ));
        if (length == 0) {
            length = 1; // In case we get 000 velocity without this normalization would produce -nan
        }
        if (length > 1.f) { // Normalize if we get velocity vector that has length > 1
            float normLength    = 1 / length;
            grenade->velocity.x = velX * normLength;
            grenade->velocity.y = velY * normLength;
            grenade->velocity.z = velZ * normLength;
        } else {
            grenade->velocity.x = velX;
            grenade->velocity.y = velY;
            grenade->velocity.z = velZ;
        }
        Vector3f posZ1 = grenade->position;
        posZ1.z += 1;
        Vector3f posZ2 = grenade->position;
        posZ2.z += 2;
        if (vecfValidPos(server, grenade->position) ||
            (vecfValidPos(server, posZ1) && server->player[playerID].movement.position.z < 0) ||
            (vecfValidPos(server, posZ2) && server->player[playerID].movement.position.z < 0))
        {
            sendGrenade(server, playerID, grenade->fuse, grenade->position, grenade->velocity);
            grenade->sent          = 1;
            grenade->timeSinceSent = getNanos();
        }
        DL_APPEND(server->player[playerID].grenade, grenade);
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

    if (server->player[playerID].sprinting ||
        (server->player[playerID].item == 2 && server->player[playerID].weaponClip <= 0))
    {
        return; // Sprinting and hitting somebody is impossible
    }

    uint64 timeNow = getNanos();

    if (allowShot(server, playerID, hitPlayerID, timeNow, distance, &x, &y, &z, shotPos, shotOrien, hitPos, shotEyePos))
    {
        switch (server->player[playerID].weapon) {
            case WEAPON_RIFLE:
            {
                switch (hitType) {
                    case HIT_TYPE_HEAD:
                    {
                        sendKillActionPacket(server, playerID, hitPlayerID, 1, 5, 0);
                        break;
                    }
                    case HIT_TYPE_TORSO:
                    {
                        sendSetHP(server, playerID, hitPlayerID, 49, 1, 0, 5, 0, shotPos);
                        break;
                    }
                    case HIT_TYPE_ARMS:
                    {
                        sendSetHP(server, playerID, hitPlayerID, 33, 1, 0, 5, 0, shotPos);
                        break;
                    }
                    case HIT_TYPE_LEGS:
                    {
                        sendSetHP(server, playerID, hitPlayerID, 33, 1, 0, 5, 0, shotPos);
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
                        sendSetHP(server, playerID, hitPlayerID, 75, 1, 1, 5, 0, shotPos);
                        break;
                    }
                    case HIT_TYPE_TORSO:
                    {
                        sendSetHP(server, playerID, hitPlayerID, 29, 1, 0, 5, 0, shotPos);
                        break;
                    }
                    case HIT_TYPE_ARMS:
                    {
                        sendSetHP(server, playerID, hitPlayerID, 18, 1, 0, 5, 0, shotPos);
                        break;
                    }
                    case HIT_TYPE_LEGS:
                    {
                        sendSetHP(server, playerID, hitPlayerID, 18, 1, 0, 5, 0, shotPos);
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
                        sendSetHP(server, playerID, hitPlayerID, 37, 1, 1, 5, 0, shotPos);
                        break;
                    }
                    case HIT_TYPE_TORSO:
                    {
                        sendSetHP(server, playerID, hitPlayerID, 27, 1, 0, 5, 0, shotPos);
                        break;
                    }
                    case HIT_TYPE_ARMS:
                    {
                        sendSetHP(server, playerID, hitPlayerID, 16, 1, 0, 5, 0, shotPos);
                        break;
                    }
                    case HIT_TYPE_LEGS:
                    {
                        sendSetHP(server, playerID, hitPlayerID, 16, 1, 0, 5, 0, shotPos);
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
            sendSetHP(server, playerID, hitPlayerID, 80, 1, 2, 5, 0, shotPos);
        }
    }
}

static void receiveOrientationData(Server* server, uint8 playerID, DataStream* data)
{
    float x, y, z;
    x                = ReadFloat(data);
    y                = ReadFloat(data);
    z                = ReadFloat(data);
    float length     = sqrt((x * x) + (y * y) + (z * z));
    float normLength = 1 / length;
    // Normalize the vectors if their length > 1
    if (length > 1.f) {
        server->player[playerID].movement.forwardOrientation.x = x * normLength;
        server->player[playerID].movement.forwardOrientation.y = y * normLength;
        server->player[playerID].movement.forwardOrientation.z = z * normLength;
    } else {
        server->player[playerID].movement.forwardOrientation.x = x;
        server->player[playerID].movement.forwardOrientation.y = y;
        server->player[playerID].movement.forwardOrientation.z = z;
    }

    reorientPlayer(server, playerID, &server->player[playerID].movement.forwardOrientation);
}

static void receiveInputData(Server* server, uint8 playerID, DataStream* data)
{
    uint8 bits[8];
    uint8 mask = 1;
    uint8 ID;
    ID = ReadByte(data);
    if (playerID != ID) {
        LOG_WARNING("Assigned ID: %d doesnt match sent ID: %d in Input packet", playerID, ID);
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
        sendInputData(server, playerID);
    }
}

static void receivePositionData(Server* server, uint8 playerID, DataStream* data)
{
    float x, y, z;
    x = ReadFloat(data);
    y = ReadFloat(data);
    z = ReadFloat(data);
#ifdef DEBUG
    LOG_DEBUG("Player: %d, Our X: %f, Y: %f, Z: %f Actual X: %f, Y: %f, Z: %f",
              playerID,
              server->player[playerID].movement.position.x,
              server->player[playerID].movement.position.y,
              server->player[playerID].movement.position.z,
              x,
              y,
              z);
    LOG_DEBUG("Player: %d, Z solid: %d, Z+1 solid: %d, Z+2 solid: %d, Z: %d, Z+1: %d, Z+2: %d, Crouching: %d",
              playerID,
              mapvxlIsSolid(&server->map.map, (int) x, (int) y, (int) z),
              mapvxlIsSolid(&server->map.map, (int) x, (int) y, (int) z + 1),
              mapvxlIsSolid(&server->map.map, (int) x, (int) y, (int) z + 2),
              (int) z,
              (int) z + 1,
              (int) z + 2,
              server->player[playerID].crouching);
#endif
    server->player[playerID].movement.prevPosition = server->player[playerID].movement.position;
    server->player[playerID].movement.position.x   = x;
    server->player[playerID].movement.position.y   = y;
    server->player[playerID].movement.position.z   = z;
    /*uint8 resetTime                                = 1;
    if (!diffIsOlderThenDontUpdate(
        getNanos(), server->player[playerID].timers.duringNoclipPeriod, (uint64) NANO_IN_SECOND * 10))
    {
        resetTime = 0;
        if (validPlayerPos(server, playerID, x, y, z) == 0) {
            server->player[playerID].invalidPosCount++;
            if (server->player[playerID].invalidPosCount == 5) {
                sendMessageToStaff(server, "Player %s (#%hhu) is most likely trying to avoid noclip detection");
            }
        }
    }
    if (resetTime) {
        server->player[playerID].timers.duringNoclipPeriod = getNanos();
        server->player[playerID].invalidPosCount           = 0;
    }*/

    if (validPlayerPos(server, playerID, x, y, z)) {
        server->player[playerID].movement.prevLegitPos = server->player[playerID].movement.position;
    } /*else if (validPlayerPos(server,
                              playerID,
                              server->player[playerID].movement.position.x,
                              server->player[playerID].movement.position.y,
                              server->player[playerID].movement.position.z) == 0 &&
               validPlayerPos(server,
                              playerID,
                              server->player[playerID].movement.prevPosition.x,
                              server->player[playerID].movement.prevPosition.y,
                              server->player[playerID].movement.prevPosition.z) == 0)
    {
        LOG_WARNING("Player %s (#%hhu) may be noclipping!", server->player[playerID].name, playerID);
        sendMessageToStaff(server, "Player %s (#%d) may be noclipping!", server->player[playerID].name, playerID);
        if (validPlayerPos(server,
                           playerID,
                           server->player[playerID].movement.prevLegitPos.x,
                           server->player[playerID].movement.prevLegitPos.y,
                           server->player[playerID].movement.prevLegitPos.z) == 0)
        {
            if (getPlayerUnstuck(server, playerID) == 0) {
                SetPlayerRespawnPoint(server, playerID);
                sendServerNotice(
                server, playerID, 0, "Server could not find a free position to get you unstuck. Reseting to spawn");
                LOG_WARNING(
                "Could not find legit position for player %s (#%d) to get them unstuck. Resetting to spawn. "
                "Go check them!",
                server->player[playerID].name,
                playerID);
                sendMessageToStaff(
                server,
                "Could not find legit position for player %s (#%d) to get them unstuck. Resetting to spawn. "
                "Go check them!",
                server->player[playerID].name,
                playerID);
                server->player[playerID].movement.prevLegitPos = server->player[playerID].movement.position;
            }
        }
        SendPositionPacket(server,
                           playerID,
                           server->player[playerID].movement.prevLegitPos.x,
                           server->player[playerID].movement.prevLegitPos.y,
                           server->player[playerID].movement.prevLegitPos.z);
    }*/
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
        LOG_WARNING("Name of player %d is too long. Cutting", playerID);
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
    char IP[17];
    formatIPToString(IP, server->player[playerID].ipStruct);
    char team[15];
    teamIDToString(server, team, server->player[playerID].team);
    LOG_INFO("Player %s (%s, #%hhu) joined %s", server->player[playerID].name, IP, playerID, team);
    if (server->player[playerID].welcomeSent == 0) {
        stringNode* welcomeMessage;
        DL_FOREACH(server->welcomeMessages, welcomeMessage)
        {
            sendServerNotice(server, playerID, 0, welcomeMessage->string);
        }
        if (invName) {
            sendServerNotice(server,
                             playerID,
                             0,
                             "Your name was either empty, had # in front of it or contained something nasty. Your name "
                             "has been set to %s",
                             server->player[playerID].name);
        }
        server->player[playerID].welcomeSent = 1; // So we dont send the message to the player on each time they spawn.
    }

    if (server->protocol.gameMode.intelHeld[0] == 0) {
        sendMoveObject(server, 0, 0, server->protocol.gameMode.intel[0]);
    }
    if (server->protocol.gameMode.intelHeld[1] == 0) {
        sendMoveObject(server, 1, 1, server->protocol.gameMode.intel[1]);
    }
}

static void receiveBlockAction(Server* server, uint8 playerID, DataStream* data)
{
    uint8 ID = ReadByte(data);
    if (playerID != ID) {
        LOG_WARNING("Assigned ID: %d doesnt match sent ID: %d in block action packet", playerID, ID);
    }
    if (server->player[playerID].canBuild && server->globalAB) {
        uint8  actionType = ReadByte(data);
        uint32 X          = ReadInt(data);
        uint32 Y          = ReadInt(data);
        uint32 Z          = ReadInt(data);
        if (server->player[playerID].sprinting) {
            return;
        }
        Vector3i vectorBlock  = {X, Y, Z};
        Vector3f vectorfBlock = {(float) X, (float) Y, (float) Z};
        Vector3f playerVector = server->player[playerID].movement.position;
        if (((server->player[playerID].item == 0 && (actionType == 1 || actionType == 2)) ||
             (server->player[playerID].item == 1 && actionType == 0) ||
             (server->player[playerID].item == 2 && actionType == 1)))
        {
            if ((DistanceIn3D(vectorfBlock, playerVector) <= 4 || server->player[playerID].item == 2) &&
                vecValidPos(server, vectorBlock))
            {
                switch (actionType) {
                    case 0:
                    {
                        if (gamemodeBlockChecks(server, X, Y, Z)) {
                            uint64 timeNow = getNanos();
                            if (server->player[playerID].blocks > 0 &&
                                diffIsOlderThen(
                                timeNow, &server->player[playerID].timers.sinceLastBlockPlac, BLOCK_DELAY) &&
                                diffIsOlderThenDontUpdate(
                                timeNow, server->player[playerID].timers.sinceLastBlockDest, BLOCK_DELAY) &&
                                diffIsOlderThenDontUpdate(
                                timeNow, server->player[playerID].timers.sinceLast3BlockDest, BLOCK_DELAY))
                            {
                                mapvxlSetColor(&server->map.map, X, Y, Z, server->player[playerID].toolColor.color);
                                server->player[playerID].blocks--;
                                moveIntelAndTentUp(server);
                                sendBlockAction(server, playerID, actionType, X, Y, Z);
                            }
                        }
                    } break;

                    case 1:
                    {
                        if (Z < 62 && gamemodeBlockChecks(server, X, Y, Z)) {
                            uint64 timeNow = getNanos();
                            if ((diffIsOlderThen(
                                 timeNow, &server->player[playerID].timers.sinceLastBlockDest, SPADE_DELAY) &&
                                 diffIsOlderThenDontUpdate(
                                 timeNow, server->player[playerID].timers.sinceLast3BlockDest, SPADE_DELAY) &&
                                 diffIsOlderThenDontUpdate(
                                 timeNow, server->player[playerID].timers.sinceLastBlockPlac, SPADE_DELAY)) ||
                                server->player[playerID].item == 2)
                            {
                                if (server->player[playerID].item == 2) {
                                    if (server->player[playerID].weaponClip <= 0) {
                                        sendMessageToStaff(server,
                                                           "Player %s (#%hhu) probably has hack to have more ammo",
                                                           server->player[playerID].name,
                                                           playerID);
                                        return;
                                    } else if (server->player[playerID].weapon == 0 &&
                                               diffIsOlderThen(
                                               timeNow,
                                               &server->player[playerID].timers.sinceLastBlockDestWithGun,
                                               ((RIFLE_DELAY * 2) - (NANO_IN_MILLI * 10))) == 0)
                                    {
                                        sendMessageToStaff(server,
                                                           "Player %s (#%hhu) probably has rapid shooting hack",
                                                           server->player[playerID].name,
                                                           playerID);
                                        return;
                                    } else if (server->player[playerID].weapon == 1 &&
                                               diffIsOlderThen(
                                               timeNow,
                                               &server->player[playerID].timers.sinceLastBlockDestWithGun,
                                               ((SMG_DELAY * 3) - (NANO_IN_MILLI * 10))) == 0)
                                    {
                                        sendMessageToStaff(server,
                                                           "Player %s (#%hhu) probably has rapid shooting hack",
                                                           server->player[playerID].name,
                                                           playerID);
                                        return;
                                    } else if (server->player[playerID].weapon == 2 &&
                                               diffIsOlderThen(
                                               timeNow,
                                               &server->player[playerID].timers.sinceLastBlockDestWithGun,
                                               (SHOTGUN_DELAY - (NANO_IN_MILLI * 10))) == 0)
                                    {
                                        sendMessageToStaff(server,
                                                           "Player %s (#%hhu) probably has rapid shooting hack",
                                                           server->player[playerID].name,
                                                           playerID);
                                        return;
                                    }
                                }

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
                                sendBlockAction(server, playerID, actionType, X, Y, Z);
                            }
                        }
                    } break;

                    case 2:
                    {
                        if (gamemodeBlockChecks(server, X, Y, Z) && gamemodeBlockChecks(server, X, Y, Z + 1) &&
                            gamemodeBlockChecks(server, X, Y, Z - 1))
                        {
                            uint64 timeNow = getNanos();
                            if (diffIsOlderThen(
                                timeNow, &server->player[playerID].timers.sinceLast3BlockDest, THREEBLOCK_DELAY) &&
                                diffIsOlderThenDontUpdate(
                                timeNow, server->player[playerID].timers.sinceLastBlockDest, THREEBLOCK_DELAY) &&
                                diffIsOlderThenDontUpdate(
                                timeNow, server->player[playerID].timers.sinceLastBlockPlac, THREEBLOCK_DELAY))
                            {
                                for (uint32 z = Z - 1; z <= Z + 1; z++) {
                                    if (z < 62) {
                                        mapvxlSetAir(&server->map.map, X, Y, z);
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
                                sendBlockAction(server, playerID, actionType, X, Y, Z);
                            }
                        }
                    } break;
                }
                moveIntelAndTentDown(server);
            }
        } else {
            LOG_WARNING("Player %s (#%hhu) may be using BlockExploit with Item: %d and Action: %d",
                        server->player[playerID].name,
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
        LOG_WARNING("Assigned ID: %d doesnt match sent ID: %d in blockline packet", playerID, ID);
    }
    uint64 timeNow = getNanos();
    if (server->player[playerID].blocks > 0 && server->player[playerID].canBuild && server->globalAB &&
        server->player[playerID].item == 1 &&
        diffIsOlderThen(timeNow, &server->player[playerID].timers.sinceLastBlockPlac, BLOCK_DELAY) &&
        diffIsOlderThenDontUpdate(timeNow, server->player[playerID].timers.sinceLastBlockDest, BLOCK_DELAY) &&
        diffIsOlderThenDontUpdate(timeNow, server->player[playerID].timers.sinceLast3BlockDest, BLOCK_DELAY))
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
            DistanceIn3D(startF, server->player[playerID].locAtClick) <= 4 && vecfValidPos(server, startF) &&
            vecfValidPos(server, endF))
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
            sendBlockLine(server, playerID, start, end);
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
        LOG_WARNING("Assigned ID: %d doesnt match sent ID: %d in set tool packet", playerID, ID);
    }

    server->player[playerID].item      = tool;
    server->player[playerID].reloading = 0;
    sendSetTool(server, playerID, tool);
}

static void receiveSetColor(Server* server, uint8 playerID, DataStream* data)
{
    uint8 ID = ReadByte(data);
    uint8 B  = ReadByte(data);
    uint8 G  = ReadByte(data);
    uint8 R  = ReadByte(data);
    uint8 A  = 0;

    if (playerID != ID) {
        LOG_WARNING("Assigned ID: %d doesnt match sent ID: %d in set color packet", playerID, ID);
    }

    server->player[playerID].toolColor.colorArray[A_CHANNEL] = A;
    server->player[playerID].toolColor.colorArray[R_CHANNEL] = R;
    server->player[playerID].toolColor.colorArray[G_CHANNEL] = G;
    server->player[playerID].toolColor.colorArray[B_CHANNEL] = B;
    sendSetColor(server, playerID, R, G, B);
}

static void receiveWeaponInput(Server* server, uint8 playerID, DataStream* data)
{
    uint8 mask   = 1;
    uint8 ID     = ReadByte(data);
    uint8 wInput = ReadByte(data);
    if (playerID != ID) {
        LOG_WARNING("Assigned ID: %d doesnt match sent ID: %d in weapon input packet", playerID, ID);
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
    } else if (server->player[playerID].secondary_fire && server->player[playerID].item == 0) {
        server->player[playerID].timers.sincePossibleSpadenade = getNanos();
    }

    else if (server->player[playerID].weaponClip > 0)
    {
        sendWeaponInput(server, playerID, wInput);
        uint64 timeDiff = 0;
        switch (server->player[playerID].weapon) {
            case WEAPON_RIFLE:
            {
                timeDiff = NANO_IN_MILLI * 500;
                break;
            }
            case WEAPON_SMG:
            {
                timeDiff = NANO_IN_MILLI * 110;
                break;
            }
            case WEAPON_SHOTGUN:
            {
                timeDiff = NANO_IN_MILLI * 1000;
                break;
            }
        }

        if (server->player[playerID].primary_fire &&
            diffIsOlderThen(getNanos(), &server->player[playerID].timers.sinceLastWeaponInput, timeDiff))
        {
            server->player[playerID].timers.sinceLastWeaponInput = getNanos();
            server->player[playerID].toRefill++;
            server->player[playerID].weaponClip--;
            server->player[playerID].reloading = 0;
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
                        sendServerNotice(server, i, 0, message);
                    }
                }
            }
            server->player[playerID].movement.previousOrientation =
            server->player[playerID].movement.forwardOrientation;
        }
    } else {
        // sendKillActionPacket(server, playerID, playerID, 0, 30, 0);
    }
}

static void receiveWeaponReload(Server* server, uint8 playerID, DataStream* data)
{
    uint8 ID      = ReadByte(data);
    uint8 clip    = ReadByte(data);
    uint8 reserve = ReadByte(data);
    if (playerID != ID) {
        LOG_WARNING("Assigned ID: %d doesnt match sent ID: %d in weapon reload packet", playerID, ID);
    }
    server->player[playerID].primary_fire   = 0;
    server->player[playerID].secondary_fire = 0;

    if (server->player[playerID].weaponReserve == 0 || server->player[playerID].reloading) {
        return;
    }
    server->player[playerID].reloading               = 1;
    server->player[playerID].timers.sinceReloadStart = getNanos();
    sendWeaponReload(server, playerID, 1, clip, reserve);
}

static void receiveChangeTeam(Server* server, uint8 playerID, DataStream* data)
{
    uint8 ID = ReadByte(data);
    server->protocol.teamUserCount[server->player[playerID].team]--;
    uint8 team                    = ReadByte(data);
    server->player[playerID].team = team;
    if (playerID != ID) {
        LOG_WARNING("Assigned ID: %d doesnt match sent ID: %d in change team packet", playerID, ID);
    }
    server->protocol.teamUserCount[server->player[playerID].team]++;
    sendKillActionPacket(server, playerID, playerID, 5, 5, 0);
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
        LOG_WARNING("Assigned ID: %d doesnt match sent ID: %d in change weapon packet", playerID, ID);
    }
    sendKillActionPacket(server, playerID, playerID, 6, 5, 0);
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
              (server->player[playerID].version_revision == 3 || server->player[playerID].version_revision == 5)))
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
            LOG_WARNING("Unhandled input, id %u, code %u", playerID, type);
            break;
    }
}
