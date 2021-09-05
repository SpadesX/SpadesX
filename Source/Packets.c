// Copyright DarkNeutrino 2021
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

void sendKillPacket(Server* server, uint8 killerID, uint8 playerID, uint8 killReason, uint8 respawnTime)
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
        if (isPastStateData(server, player)) {
            enet_peer_send(server->player[player].peer, 0, packet);
        }
    }
    if (killerID != playerID) {
        server->player[killerID].kills++;
    }
    server->player[playerID].deaths++;
    server->player[playerID].respawnTime        = respawnTime;
    server->player[playerID].startOfRespawnWait = time(NULL);
    server->player[playerID].state              = STATE_WAITING_FOR_RESPAWN;
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
            sendKillPacket(server, playerID, hitPlayerID, killReason, respawnTime);
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

float max(float num1, float num2)
{
    return (num1 > num2) ? num1 : num2;
}

void sendMessage(ENetEvent event, DataStream* data, Server* server, uint8 player)
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
        Player srvPlayer = server->player[player];
        char   command[30];
        sscanf(message, "%s", command);
        uint8 strLenght = strlen(command);
        for (uint8 i = 1; i < strLenght; ++i) {
            message[i] = command[i] = tolower(command[i]);
        }
        if (strcmp(command, "/kill") == 0) {
            int id = 0;
            if (sscanf(message, "%s #%d", command, &id) == 1) {
                sendKillPacket(server, player, player, 0, 5);
            } else {
                if (server->player[id].state == STATE_READY &&
                    (srvPlayer.isManager || srvPlayer.isAdmin || srvPlayer.isMod || srvPlayer.isGuard))
                {
                    sendKillPacket(server, id, id, 0, 5);
                } else {
                    sendServerNotice(server, player, "Player does not exist or isnt spawned yet");
                }
            }
        } else if (strcmp(command, "/tk") == 0 &&
                   (srvPlayer.isManager || srvPlayer.isAdmin || srvPlayer.isMod || srvPlayer.isGuard))
        {
            int ID;
            if (sscanf(message, "%s #%d", command, &ID) == 1) {
                if (ID >= 0 && ID < 31 && isPastJoinScreen(server, ID)) {
                    if (server->globalAK == 1) {
                        server->globalAK = 0;
                        broadcastServerNotice(server, "Killing has been disabled");
                    } else if (server->globalAK == 0) {
                        server->globalAK = 1;
                        broadcastServerNotice(server, "Killing has been enabled");
                    }
                } else {
                    sendServerNotice(server, player, "ID not in range or player doesnt exist");
                }
            } else {
                char broadcastDis[100] = "Killing has been disabled for player: ";
                char broadcastEna[100] = "Killing has been enabled for player: ";
                strcat(broadcastDis, server->player[ID].name);
                strcat(broadcastEna, server->player[ID].name);
                if (server->player[ID].allowKilling == 1) {
                    server->player[ID].allowKilling = 0;
                    broadcastServerNotice(server, broadcastDis);
                } else if (server->player[ID].allowKilling == 0) {
                    server->player[ID].allowKilling = 1;
                    broadcastServerNotice(server, broadcastEna);
                }
            }
        } else if (strcmp(command, "/ttk") == 0 &&
                   (srvPlayer.isManager || srvPlayer.isAdmin || srvPlayer.isMod || srvPlayer.isGuard))
        {
            int ID = 33;
            if (sscanf(message, "%s #%d", command, &ID) == 2) {
                if (ID >= 0 && ID < 31 && isPastJoinScreen(server, ID)) {
                    char broadcastDis[100] = "Team Killing has been disabled for player: ";
                    char broadcastEna[100] = "Team Killing has been enabled for player: ";
                    strcat(broadcastDis, server->player[ID].name);
                    strcat(broadcastEna, server->player[ID].name);
                    if (server->player[ID].allowTeamKilling) {
                        server->player[ID].allowTeamKilling = 0;
                        broadcastServerNotice(server, broadcastDis);
                    } else {
                        server->player[ID].allowTeamKilling = 1;
                        broadcastServerNotice(server, broadcastEna);
                    }
                } else {
                    sendServerNotice(server, player, "ID not in range or player doesnt exist");
                }
            } else {
                sendServerNotice(server, player, "You did not enter ID or entered incorrect argument");
            }
        } else if (strcmp(command, "/tb") == 0 &&
                   (srvPlayer.isManager || srvPlayer.isAdmin || srvPlayer.isMod || srvPlayer.isGuard))
        {
            int ID;
            if (sscanf(message, "%s #%d", command, &ID) == 1) {
                if (ID >= 0 && ID < 31 && isPastJoinScreen(server, ID)) {
                    if (server->globalAB == 1) {
                        server->globalAB = 0;
                        broadcastServerNotice(server, "Building has been disabled");
                    } else if (server->globalAB == 0) {
                        server->globalAB = 1;
                        broadcastServerNotice(server, "Building has been enabled");
                    }
                } else {
                    sendServerNotice(server, player, "ID not in range or player doesnt exist");
                }
            } else {
                char broadcastDis[100] = "Building has been disabled for player: ";
                char broadcastEna[100] = "Building has been enabled for player: ";
                strcat(broadcastDis, server->player[ID].name);
                strcat(broadcastEna, server->player[ID].name);
                if (server->player[ID].canBuild == 1) {
                    server->player[ID].canBuild = 0;
                    broadcastServerNotice(server, broadcastDis);
                } else if (server->player[ID].canBuild == 0) {
                    server->player[ID].canBuild = 1;
                    broadcastServerNotice(server, broadcastEna);
                }
            }
        } else if (strcmp(command, "/kick") == 0 &&
                   (srvPlayer.isManager || srvPlayer.isAdmin || srvPlayer.isMod || srvPlayer.isGuard))
        {
            int ID = 33;
            if (sscanf(message, "%s #%d", command, &ID) == 2) {
                if (ID >= 0 && ID < 31 && isPastJoinScreen(server, ID)) {
                    char sendingMessage[100];
                    strcpy(sendingMessage, server->player[ID].name);
                    strcat(sendingMessage, " has been kicked");
                    enet_peer_disconnect(server->player[ID].peer, REASON_KICKED);
                    broadcastServerNotice(server, sendingMessage);
                } else {
                    sendServerNotice(server, player, "ID not in range or player doesnt exist");
                }
            } else {
                sendServerNotice(server, player, "You did not enter ID or entered incorrect argument");
            }
        } else if (strcmp(command, "/mute") == 0 &&
                   (srvPlayer.isManager || srvPlayer.isAdmin || srvPlayer.isMod || srvPlayer.isGuard))
        {
            int ID = 33;
            if (sscanf(message, "%s #%d", command, &ID) == 2) {
                if (ID >= 0 && ID < 31 && isPastJoinScreen(server, ID)) {
                    char sendingMessage[100];
                    strcpy(sendingMessage, server->player[ID].name);
                    if (server->player[ID].muted) {
                        server->player[ID].muted = 0;
                        strcat(sendingMessage, " has been unmuted");
                    } else {
                        server->player[ID].muted = 1;
                        strcat(sendingMessage, " has been muted");
                    }
                    broadcastServerNotice(server, sendingMessage);
                } else {
                    sendServerNotice(server, player, "ID not in range or player doesnt exist");
                }
            } else {
                sendServerNotice(server, player, "You did not enter ID or entered incorrect argument");
            }
        } else if (strcmp(command, "/ups") == 0) {
            float ups = 0;
            char  upsString[4];
            sscanf(message, "%s %f", command, &ups);
            sscanf(message, "%s %s", command, upsString);
            if (ups >= 1 && ups <= 300) {
                server->player[player].ups = ups;
                char fullString[32]        = "UPS changed to ";
                strcat(fullString, upsString);
                strcat(fullString, " succesufully");
                sendServerNotice(server, player, fullString);
            } else {
                sendServerNotice(server, player, "Changing UPS failed. Please select value between 1 and 300");
            }
        } else if (strcmp(command, "/pban") == 0 &&
                   (srvPlayer.isManager || srvPlayer.isAdmin || srvPlayer.isMod || srvPlayer.isGuard))
        {
            int ID = 33;
            if (sscanf(message, "%s #%d", command, &ID) == 2) {
                if (ID >= 0 && ID < 31 && isPastJoinScreen(server, ID)) {
                    char sendingMessage[100];
                    strcpy(sendingMessage, server->player[ID].name);
                    strcat(sendingMessage, " has been banned");
                    FILE* fp;
                    fp = fopen("BanList.txt", "a");
                    if (fp == NULL) {
                        sendServerNotice(server, player, "Player could not be banned. File failed to open");
                        return;
                    }
                    fprintf(fp, "%d %s\n", server->player[ID].peer->address.host, server->player[ID].name);
                    fclose(fp);
                    enet_peer_disconnect(server->player[ID].peer, REASON_BANNED);
                    broadcastServerNotice(server, sendingMessage);
                } else {
                    sendServerNotice(server, player, "ID not in range or player doesnt exist");
                }
            } else {
                sendServerNotice(server, player, "You did not enter ID or entered incorrect argument");
            }
        } else if (strcmp(command, "/login") == 0) {
            if (!server->player[player].isManager && !server->player[player].isAdmin && !server->player[player].isMod &&
                !server->player[player].isGuard && !server->player[player].isTrusted)
            {
                char level[32];
                char password[64];
                if (sscanf(message, "%s %s %s", command, level, password) == 3) {
                    uint8 levelLen = strlen(level);
                    for (uint8 j = 0; j < levelLen; ++j) {
                        level[j] = tolower(level[j]);
                    }
                    if (strcmp(level, "manager") == 0) {
                        if (strcmp(password, server->managerPasswd) == 0) {
                            server->player[player].isManager = 1;
                            sendServerNotice(server, player, "You have logged in as manager");
                        } else {
                            sendServerNotice(server, player, "Incorrect password");
                        }
                    } else if (strcmp(level, "admin") == 0) {
                        if (strcmp(password, server->adminPasswd) == 0) {
                            server->player[player].isAdmin = 1;
                            sendServerNotice(server, player, "You have logged in as admin");
                        } else {
                            sendServerNotice(server, player, "Incorrect password");
                        }
                    } else if (strcmp(level, "mod") == 0 || strcmp(level, "moderator") == 0) {
                        if (strcmp(password, server->modPasswd) == 0) {
                            server->player[player].isMod = 1;
                            sendServerNotice(server, player, "You have logged in as moderator");
                        } else {
                            sendServerNotice(server, player, "Incorrect password");
                        }
                    } else if (strcmp(level, "guard") == 0) {
                        if (strcmp(password, server->guardPasswd) == 0) {
                            server->player[player].isGuard = 1;
                            sendServerNotice(server, player, "You have logged in as guard");
                        } else {
                            sendServerNotice(server, player, "Incorrect password");
                        }
                    } else if (strcmp(level, "trusted") == 0) {
                        if (strcmp(password, server->trustedPasswd) == 0) {
                            server->player[player].isTrusted = 1;
                            sendServerNotice(server, player, "You have logged in as trsuted");
                        } else {
                            sendServerNotice(server, player, "Incorrect password");
                        }
                    } else {
                        sendServerNotice(server, player, "Incorrect role");
                    }
                } else {
                    sendServerNotice(server, player, "Incorrect number of arguments to login");
                }
            } else {
                sendServerNotice(server, player, "You are already logged in");
            }
        } else if (strcmp(command, "/ratio") == 0) {
            int ID = 33;
            if (sscanf(message, "%s #%d", command, &ID) == 2) {
                if (ID >= 0 && ID < 31 && isPastJoinScreen(server, ID)) {
                    char sendingMessage[100];
                    char ratioInString[12];
                    char killsInString[4];
                    char deathsInString[4];
                    gcvt(
                    ((float) server->player[ID].kills / max(1, (float) server->player[ID].deaths)), 7, ratioInString);
                    sprintf(killsInString, "%d", server->player[ID].kills);
                    sprintf(deathsInString, "%d", server->player[ID].deaths);
                    strcpy(sendingMessage, server->player[ID].name);
                    strcat(sendingMessage, " has kill to death ratio of: ");
                    strcat(sendingMessage, ratioInString);
                    strcat(sendingMessage, " (Kills: ");
                    strcat(sendingMessage, killsInString);
                    strcat(sendingMessage, ", Deaths: ");
                    strcat(sendingMessage, deathsInString);
                    strcat(sendingMessage, ")");
                    sendServerNotice(server, player, sendingMessage);
                } else {
                    sendServerNotice(server, player, "ID not in range or player doesnt exist");
                }
            } else {
                char sendingMessage[100];
                char ratioInString[12];
                char killsInString[4];
                char deathsInString[4];
                gcvt(((float) server->player[player].kills / max(1, (float) server->player[player].deaths)),
                     7,
                     ratioInString);
                sprintf(killsInString, "%d", server->player[player].kills);
                sprintf(deathsInString, "%d", server->player[player].deaths);
                strcpy(sendingMessage, server->player[player].name);
                strcat(sendingMessage, " has kill to death ratio of: ");
                strcat(sendingMessage, ratioInString);
                strcat(sendingMessage, " (Kills: ");
                strcat(sendingMessage, killsInString);
                strcat(sendingMessage, ", Deaths: ");
                strcat(sendingMessage, deathsInString);
                strcat(sendingMessage, ")");
                sendServerNotice(server, player, sendingMessage);
            }
        }
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

    for (uint8 j = 0; j < 32; ++j) {
        if (playerToPlayerVisible(server, playerID, j)) {
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
