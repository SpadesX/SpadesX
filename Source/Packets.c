// Copyright DarkNeutrino 2021
#include "Commands.h"
#include "Enums.h"
#include "Protocol.h"
#include "Structs.h"

#include <Compress.h>
#include <DataStream.h>
#include <Queue.h>
#include <Types.h>
#include <ctype.h>
#include <enet/enet.h>
#include <libmapvxl/libmapvxl.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

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
    ENetPacket* packet = enet_packet_create(NULL, 3, ENET_PACKET_FLAG_RELIABLE);
    DataStream  stream = {packet->data, packet->dataLength, 0};
    WriteByte(&stream, PACKET_TYPE_INTEL_CAPTURE);
    WriteByte(&stream, playerID);
    WriteByte(&stream, winning);
    for (int player = 0; player < server->protocol.maxPlayers; ++player) {
        if (isPastStateData(server, player)) {
            enet_peer_send(server->player[player].peer, 0, packet);
        }
    }
}

void SendIntelPickup(Server* server, uint8 playerID)
{
    ENetPacket* packet = enet_packet_create(NULL, 2, ENET_PACKET_FLAG_RELIABLE);
    DataStream  stream = {packet->data, packet->dataLength, 0};
    WriteByte(&stream, PACKET_TYPE_INTEL_PICKUP);
    WriteByte(&stream, playerID);
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
    ENetPacket* packet = enet_packet_create(NULL, 14, ENET_PACKET_FLAG_RELIABLE);
    DataStream  stream = {packet->data, packet->dataLength, 0};
    WriteByte(&stream, PACKET_TYPE_INTEL_DROP);
    WriteByte(&stream, playerID);
    WriteFloat(&stream, server->player[playerID].movement.position.x);
    WriteFloat(&stream, server->player[playerID].movement.position.y);
    WriteFloat(&stream, floor(server->player[playerID].movement.position.z + 3));
    server->protocol.ctf.intel[team].x = (int) server->player[playerID].movement.position.x;
    server->protocol.ctf.intel[team].y = (int) server->player[playerID].movement.position.y;
    server->protocol.ctf.intel[team].z = (int) server->player[playerID].movement.position.z + 3;
    printf("Dropping intel at X: %d, Y: %d, Z: %d\n",
           (int) server->protocol.ctf.intel[team].x,
           (int) server->protocol.ctf.intel[team].y,
           (int) server->protocol.ctf.intel[team].z);
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
    SendPacketExceptSender(server, packet, playerID);
}

void SendPlayerLeft(Server* server, uint8 playerID)
{
    printf("Player %s disconnected\n", server->player[playerID].name);
    for (uint8 i = 0; i < server->protocol.maxPlayers; ++i) {
        if (i != playerID && isPastStateData(server, i)) {
            ENetPacket* packet = enet_packet_create(NULL, 2, ENET_PACKET_FLAG_RELIABLE);
            packet->data[0]    = PACKET_TYPE_PLAYER_LEFT;
            packet->data[1]    = playerID;

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
    WriteByte(&stream, server->player[playerID].weaponReserve);
    WriteByte(&stream, server->player[playerID].weaponClip);
    SendPacketExceptSender(server, packet, playerID);
}

void SendWeaponInput(Server* server, uint8 playerID, uint8 wInput)
{
    ENetPacket* packet = enet_packet_create(NULL, 3, ENET_PACKET_FLAG_RELIABLE);
    DataStream  stream = {packet->data, packet->dataLength, 0};
    WriteByte(&stream, PACKET_TYPE_WEAPON_INPUT);
    WriteByte(&stream, playerID);
    WriteByte(&stream, wInput);
    SendPacketExceptSender(server, packet, playerID);
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
    SendPacketExceptSender(server, packet, playerID);
}

void SendSetTool(Server* server, uint8 playerID, uint8 tool)
{
    ENetPacket* packet = enet_packet_create(NULL, 3, ENET_PACKET_FLAG_RELIABLE);
    DataStream  stream = {packet->data, packet->dataLength, 0};
    WriteByte(&stream, PACKET_TYPE_SET_TOOL);
    WriteByte(&stream, playerID);
    WriteByte(&stream, tool);
    SendPacketExceptSender(server, packet, playerID);
}

void SendBlockLine(Server* server, uint8 playerID, vec3i start, vec3i end)
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
    WriteColor3i(&stream, server->protocol.colorTeamA);
    WriteColor3i(&stream, server->protocol.colorTeamB);
    WriteArray(&stream, server->protocol.nameTeamA, 10);
    WriteArray(&stream, server->protocol.nameTeamB, 10);
    WriteByte(&stream, server->protocol.mode);

    // MODE CTF:

    WriteByte(&stream, server->protocol.ctf.score[0]);   // SCORE TEAM A
    WriteByte(&stream, server->protocol.ctf.score[1]);   // SCORE TEAM B
    WriteByte(&stream, server->protocol.ctf.scoreLimit); // SCORE LIMIT
    WriteByte(&stream, server->protocol.ctf.intelFlags); // INTEL FLAGS

    if ((server->protocol.ctf.intelFlags & 1) == 0) {
        WriteVector3f(&stream, server->protocol.ctf.intel[0]);
    } else {
        WriteByte(&stream, server->protocol.ctf.playerIntelTeamA);
        StreamSkip(&stream, 11);
    }

    if ((server->protocol.ctf.intelFlags & 2) == 0) {
        WriteVector3f(&stream, server->protocol.ctf.intel[1]);
    } else {
        WriteByte(&stream, server->protocol.ctf.playerIntelTeamB);
        StreamSkip(&stream, 11);
    }

    WriteVector3f(&stream, server->protocol.ctf.base[0]);
    WriteVector3f(&stream, server->protocol.ctf.base[1]);

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
    SendPacketExceptSenderDistCheck(server, packet, playerID);
}

void sendKillPacket(Server* server,
                    uint8   killerID,
                    uint8   playerID,
                    uint8   killReason,
                    uint8   respawnTime,
                    uint8   makeInvisible)
{
    uint8 team;
    if (server->player[playerID].team == 0) {
        team = 1;
    } else {
        team = 0;
    }
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
    if (!makeInvisible) {
        if (killerID != playerID) {
            server->player[killerID].kills++;
        }
        server->player[playerID].deaths++;
        server->player[playerID].respawnTime        = respawnTime;
        server->player[playerID].startOfRespawnWait = time(NULL);
        server->player[playerID].state              = STATE_WAITING_FOR_RESPAWN;
    }
    if (server->player[playerID].hasIntel) {
        server->player[playerID].hasIntel    = 0;
        server->protocol.ctf.intelHeld[team] = 0;
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
            WriteFloat(&stream, server->player[playerID].movement.position.x);
            WriteFloat(&stream, server->player[playerID].movement.position.y);
            WriteFloat(&stream, server->player[playerID].movement.position.z);
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
        uint8* out = (uint8*) malloc(server->map.mapSize);
        mapvxlWriteMap(&server->map.map, out);
        server->map.compressedMap       = CompressData(out, server->map.mapSize, DEFAULT_COMPRESSOR_CHUNK_SIZE);
        server->player[playerID].queues = server->map.compressedMap;
    }
}

void SendMapChunks(Server* server, uint8 playerID)
{
    if (server->player[playerID].queues == NULL) {
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
    if (message[0] == '/') {
        handleCommands(server, player, message);
    } else {
        for (int playerID = 0; playerID < server->protocol.maxPlayers; ++playerID) {
            if (isPastJoinScreen(server, playerID) && !server->player[player].muted &&
                ((server->player[playerID].team == server->player[player].team && meantfor == 1) || meantfor == 0))
            {
                enet_peer_send(server->player[playerID].peer, 0, packet);
            }
        }
    }
    free(message);
}

void SendWorldUpdate(Server* server, uint8 playerID)
{
    ENetPacket* packet = enet_packet_create(NULL, 1 + (32 * 24), ENET_PACKET_FLAG_UNSEQUENCED);
    DataStream  stream = {packet->data, packet->dataLength, 0};
    WriteByte(&stream, PACKET_TYPE_WORLD_UPDATE);

    for (uint8 j = 0; j < server->protocol.maxPlayers; ++j) {
        if (playerToPlayerVisible(server, playerID, j) && server->player[j].isInvisible == 0) {
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
