// Copyright DarkNeutrino 2021
#include "Server.h"

#include "Conversion.h"
#include "Enums.h"
#include "Map.h"
#include "Master.h"
#include "PacketReceive.h"
#include "Packets.h"
#include "Protocol.h"
#include "Structs.h"

#include <Compress.h>
#include <DataStream.h>
#include <Line.h>
#include <Queue.h>
#include <Types.h>
#include <enet/enet.h>
#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

unsigned long long updateTime;
unsigned long long lastUpdateTime;
unsigned long long timeSinceStart;
Server             server;
pthread_t          thread[3];

static unsigned long long get_nanos(void)
{
    struct timespec ts;
    timespec_get(&ts, TIME_UTC);
    return (unsigned long long) ts.tv_sec * 1000000000L + ts.tv_nsec;
}

static void* calculatePhysics()
{
    updateTime = get_nanos();
    if (updateTime - lastUpdateTime >= (1000000000 / 120)) {
        updateMovementAndGrenades(&server, updateTime, lastUpdateTime, timeSinceStart);
        lastUpdateTime = get_nanos();
    }
    return 0;
}

static void ServerInit(Server*     server,
                       uint32      connections,
                       char        mapArray[][64],
                       uint8       mapCount,
                       char*       serverName,
                       char*       team1Name,
                       char*       team2Name,
                       uint8*      team1Color,
                       uint8*      team2Color,
                       uint8       gamemode)
{
    updateTime = lastUpdateTime = get_nanos();
    server->protocol.numPlayers = 0;
    server->protocol.maxPlayers = (connections <= 32) ? connections : 32;

    server->map.compressedMap   = NULL;
    server->map.compressedSize  = 0;
    server->protocol.inputFlags = 0;

    for (int i = 0; i < mapCount; ++i) {
        memcpy(server->map.mapArray, mapArray[i], strlen(mapArray[i]));
    }
    server->map.mapCount = mapCount;

    char map[64];
    srand(time(0));
    uint8 index = rand() % mapCount;
    server->map.mapIndex = index;
    memcpy(map, mapArray[index], strlen(mapArray[index]) + 1);
    printf("STATUS: Selecting %s as map\n", map);

    Vector3f empty   = {0, 0, 0};
    Vector3f forward = {1, 0, 0};
    Vector3f height  = {0, 0, 1};
    Vector3f strafe  = {0, 1, 0};
    for (uint32 i = 0; i < server->protocol.maxPlayers; ++i) {
        server->player[i].state                       = STATE_DISCONNECTED;
        server->player[i].queues                      = NULL;
        server->player[i].ups                         = 60;
        server->player[i].timeSinceLastWU             = get_nanos();
        server->player[i].input                       = 0;
        server->player[i].movement.eyePos             = empty;
        server->player[i].movement.forwardOrientation = forward;
        server->player[i].movement.strafeOrientation  = strafe;
        server->player[i].movement.heightOrientation  = height;
        server->player[i].movement.position           = empty;
        server->player[i].movement.velocity           = empty;
        server->player[i].airborne                    = 0;
        server->player[i].wade                        = 0;
        server->player[i].lastclimb                   = 0;
        server->player[i].movBackwards                = 0;
        server->player[i].movForward                  = 0;
        server->player[i].movLeft                     = 0;
        server->player[i].movRight                    = 0;
        server->player[i].jumping                     = 0;
        server->player[i].crouching                   = 0;
        server->player[i].sneaking                    = 0;
        server->player[i].sprinting                   = 0;
        server->player[i].primary_fire                = 0;
        server->player[i].secondary_fire              = 0;
        server->player[i].canBuild                    = 1;
        server->player[i].allowKilling                = 1;
        server->player[i].allowTeamKilling            = 0;
        server->player[i].muted                       = 0;
        server->player[i].toldToMaster                = 0;
        server->player[i].sinceLastBaseEnter          = time(NULL);
        server->player[i].sinceLastBaseEnterRestock   = time(NULL);
        server->player[i].HP                          = 100;
        server->player[i].blocks                      = 50;
        server->player[i].grenades                    = 3;
        server->player[i].hasIntel                    = 0;
        server->player[i].isManager                   = 0;
        server->player[i].isAdmin                     = 0;
        server->player[i].isMod                       = 0;
        server->player[i].isGuard                     = 0;
        server->player[i].isTrusted                   = 0;
        server->player[i].isInvisible                 = 0;
        server->player[i].kills                       = 0;
        server->player[i].deaths                      = 0;
        memset(server->player[i].name, 0, 17);
    }

    server->globalAB                  = 1;
    server->globalAK                  = 1;
    server->protocol.spawns[0].from.x = 64.f;
    server->protocol.spawns[0].from.y = 224.f;
    server->protocol.spawns[0].to.x   = 128.f;
    server->protocol.spawns[0].to.y   = 288.f;

    server->protocol.spawns[1].from.x = 382.f;
    server->protocol.spawns[1].from.y = 224.f;
    server->protocol.spawns[1].to.x   = 448.f;
    server->protocol.spawns[1].to.y   = 288.f;

    server->protocol.colorFog[0] = 0x80;
    server->protocol.colorFog[1] = 0xE8;
    server->protocol.colorFog[2] = 0xFF;

    server->protocol.colorTeamA[0] = team1Color[0];
    server->protocol.colorTeamA[1] = team1Color[1];
    server->protocol.colorTeamA[2] = team1Color[2];

    server->protocol.colorTeamB[0] = team2Color[0];
    server->protocol.colorTeamB[1] = team2Color[1];
    server->protocol.colorTeamB[2] = team2Color[2];

    memcpy(server->protocol.nameTeamA, team1Name, strlen(team1Name));
    memcpy(server->protocol.nameTeamB, team2Name, strlen(team2Name));
    server->protocol.nameTeamA[strlen(team1Name)] = '\0';
    server->protocol.nameTeamB[strlen(team2Name)] = '\0';
    memcpy(server->serverName, serverName, strlen(serverName));
    server->serverName[strlen(serverName)] = '\0';
    memcpy(server->mapName, map, strlen(map) - 4);
    server->mapName[strlen(server->mapName)] = '\0';

    if (gamemode == 0) {
        server->protocol.mode = GAME_MODE_CTF;
    } else if (gamemode == 1) {
        server->protocol.mode = GAME_MODE_TC;
    }

    if (server->protocol.mode == GAME_MODE_CTF) {
        memcpy(server->gamemodeName, "ctf", strlen("ctf") + 1);
    } else if (server->protocol.mode == GAME_MODE_TC) {
        memcpy(server->gamemodeName, "tc", strlen("tc") + 1);
    } else {
        memcpy(server->gamemodeName, "IDK", strlen("IDK") + 1);
    }

    // Init CTF

    server->protocol.ctf.score[0]   = 0;
    server->protocol.ctf.score[1]   = 0;
    server->protocol.ctf.scoreLimit = 10;
    server->protocol.ctf.intelFlags = 0;
    // intel
    server->protocol.ctf.intel[0]     = SetIntelTentSpawnPoint(server, 0);
    server->protocol.ctf.intel[1]     = SetIntelTentSpawnPoint(server, 1);
    server->protocol.ctf.intelHeld[0] = 0;
    server->protocol.ctf.intelHeld[1] = 0;
    // bases
    server->protocol.ctf.base[0] = SetIntelTentSpawnPoint(server, 0);
    server->protocol.ctf.base[1] = SetIntelTentSpawnPoint(server, 1);

    server->protocol.ctf.base[0].x = floorf(server->protocol.ctf.base[0].x);
    server->protocol.ctf.base[0].y = floorf(server->protocol.ctf.base[0].y);
    server->protocol.ctf.base[1].x = floorf(server->protocol.ctf.base[1].x);
    server->protocol.ctf.base[1].y = floorf(server->protocol.ctf.base[1].y);

    LoadMap(server, map);
}

void ServerReset(Server* server)
{
    updateTime = lastUpdateTime = get_nanos();

    server->map.compressedMap   = NULL;
    server->map.compressedSize  = 0;
    server->protocol.inputFlags = 0;

    char map[64];
    uint8 index = rand() % server->map.mapCount;
    server->map.mapIndex = index;
    memcpy(map, server->map.mapArray[index], strlen(server->map.mapArray[index]) + 1);
    printf("STATUS: Selecting %s as map\n", map);

    Vector3f empty   = {0, 0, 0};
    Vector3f forward = {1, 0, 0};
    Vector3f height  = {0, 0, 1};
    Vector3f strafe  = {0, 1, 0};
    for (uint32 i = 0; i < server->protocol.maxPlayers; ++i) {
        server->player[i].ups                         = 60;
        server->player[i].timeSinceLastWU             = get_nanos();
        server->player[i].input                       = 0;
        server->player[i].movement.eyePos             = empty;
        server->player[i].movement.forwardOrientation = forward;
        server->player[i].movement.strafeOrientation  = strafe;
        server->player[i].movement.heightOrientation  = height;
        server->player[i].movement.position           = empty;
        server->player[i].movement.velocity           = empty;
        server->player[i].airborne                    = 0;
        server->player[i].wade                        = 0;
        server->player[i].lastclimb                   = 0;
        server->player[i].movBackwards                = 0;
        server->player[i].movForward                  = 0;
        server->player[i].movLeft                     = 0;
        server->player[i].movRight                    = 0;
        server->player[i].jumping                     = 0;
        server->player[i].crouching                   = 0;
        server->player[i].sneaking                    = 0;
        server->player[i].sprinting                   = 0;
        server->player[i].primary_fire                = 0;
        server->player[i].secondary_fire              = 0;
        server->player[i].canBuild                    = 1;
        server->player[i].allowKilling                = 1;
        server->player[i].allowTeamKilling            = 0;
        server->player[i].muted                       = 0;
        server->player[i].toldToMaster                = 0;
        server->player[i].sinceLastBaseEnter          = time(NULL);
        server->player[i].sinceLastBaseEnterRestock   = time(NULL);
        server->player[i].HP                          = 100;
        server->player[i].blocks                      = 50;
        server->player[i].grenades                    = 3;
        server->player[i].hasIntel                    = 0;
        server->player[i].isInvisible                 = 0;
        server->player[i].kills                       = 0;
        server->player[i].deaths                      = 0;
        memset(server->player[i].name, 0, 17);
    }
    memcpy(server->mapName, map, strlen(map) - 4);
    server->mapName[strlen(server->mapName)] = '\0';

    server->globalAB                  = 1;
    server->globalAK                  = 1;
    server->protocol.spawns[0].from.x = 64.f;
    server->protocol.spawns[0].from.y = 224.f;
    server->protocol.spawns[0].to.x   = 128.f;
    server->protocol.spawns[0].to.y   = 288.f;

    server->protocol.spawns[1].from.x = 382.f;
    server->protocol.spawns[1].from.y = 224.f;
    server->protocol.spawns[1].to.x   = 448.f;
    server->protocol.spawns[1].to.y   = 288.f;

    server->protocol.colorFog[0] = 0x80;
    server->protocol.colorFog[1] = 0xE8;
    server->protocol.colorFog[2] = 0xFF;

    server->protocol.mode = GAME_MODE_CTF;

    // Init CTF

    server->protocol.ctf.score[0]   = 0;
    server->protocol.ctf.score[1]   = 0;
    server->protocol.ctf.scoreLimit = 10;
    server->protocol.ctf.intelFlags = 0;
    // intel
    server->protocol.ctf.intel[0]     = SetIntelTentSpawnPoint(server, 0);
    server->protocol.ctf.intel[1]     = SetIntelTentSpawnPoint(server, 1);
    server->protocol.ctf.intelHeld[0] = 0;
    server->protocol.ctf.intelHeld[1] = 0;
    // bases
    server->protocol.ctf.base[0] = SetIntelTentSpawnPoint(server, 0);
    server->protocol.ctf.base[1] = SetIntelTentSpawnPoint(server, 1);

    server->protocol.ctf.base[0].x = floorf(server->protocol.ctf.base[0].x);
    server->protocol.ctf.base[0].y = floorf(server->protocol.ctf.base[0].y);
    server->protocol.ctf.base[1].x = floorf(server->protocol.ctf.base[1].x);
    server->protocol.ctf.base[1].y = floorf(server->protocol.ctf.base[1].y);

    LoadMap(server, server->mapName);
}

static void SendJoiningData(Server* server, uint8 playerID)
{
    STATUS("sending state");
    for (uint8 i = 0; i < server->protocol.maxPlayers; ++i) {
        if (i != playerID && isPastJoinScreen(server, i)) {
            SendPlayerState(server, playerID, i);
        }
    }
    SendStateData(server, playerID);
}

static uint8 OnConnect(Server* server)
{
    if (server->protocol.numPlayers == server->protocol.maxPlayers) {
        return 0xFF;
    }
    uint8 playerID;
    for (playerID = 0; playerID < server->protocol.maxPlayers; ++playerID) {
        if (server->player[playerID].state == STATE_DISCONNECTED) {
            server->player[playerID].state = STATE_STARTING_MAP;
            break;
        }
    }
    server->protocol.numPlayers++;
    return playerID;
}

static void OnPlayerUpdate(Server* server, uint8 playerID)
{
    switch (server->player[playerID].state) {
        case STATE_STARTING_MAP:
            SendMapStart(server, playerID);
            break;
        case STATE_LOADING_CHUNKS:
            SendMapChunks(server, playerID);
            break;
        case STATE_JOINING:
            SendJoiningData(server, playerID);
            break;
        case STATE_SPAWNING:
            server->player[playerID].HP       = 100;
            server->player[playerID].grenades = 3;
            server->player[playerID].blocks   = 50;
            server->player[playerID].alive    = 1;
            SetPlayerRespawnPoint(server, playerID);
            SendRespawn(server, playerID);
            break;
        case STATE_WAITING_FOR_RESPAWN:
        {
            if (time(NULL) - server->player[playerID].startOfRespawnWait >= server->player[playerID].respawnTime) {
                server->player[playerID].state = STATE_SPAWNING;
            }
            break;
        }
        case STATE_READY:
            // send data
            if (server->master.enableMasterConnection == 1) {
                if (server->player[playerID].toldToMaster == 0) {
                    updateMaster(server);
                    server->player[playerID].toldToMaster = 1;
                }
            }
            break;
        default:
            // disconnected
            break;
    }
}

static void* WorldUpdate()
{
    for (uint8 playerID = 0; playerID < server.protocol.maxPlayers; ++playerID) {
        OnPlayerUpdate(&server, playerID);
        if (isPastJoinScreen(&server, playerID)) {
            unsigned long long time = get_nanos();
            if (time - server.player[playerID].timeSinceLastWU >= (1000000000 / server.player[playerID].ups)) {
                SendWorldUpdate(&server, playerID);
                server.player[playerID].timeSinceLastWU = get_nanos();
            }
        }
    }
    return 0;
}

static void ServerUpdate(Server* server, int timeout)
{
    ENetEvent event;
    while (enet_host_service(server->host, &event, timeout) > 0) {
        uint8 bannedUser = 0;
        uint8 playerID;
        switch (event.type) {
            case ENET_EVENT_TYPE_NONE:
                STATUS("Event of type none received. Ignoring");
                break;
            case ENET_EVENT_TYPE_CONNECT:
                if (event.data != VERSION_0_75) {
                    enet_peer_disconnect_now(event.peer, REASON_WRONG_PROTOCOL_VERSION);
                    break;
                }

                FILE* fp;
                fp = fopen("BanList.txt", "r");
                if (fp == NULL) {
                    WARNING(
                    "BanList.txt could not be opened for checking ban. PLEASE FIX THIS NOW BY CREATING THIS FILE!!!!");
                }
                unsigned int IP = 0;
                char         nameOfPlayer[20];
                while (fscanf(fp, "%u %s", &IP, nameOfPlayer) != EOF) {
                    if (IP == event.peer->address.host) {
                        enet_peer_disconnect_now(event.peer, REASON_BANNED);
                        printf("WARNING: Banned user %s tried to join. IP: %d\n", nameOfPlayer, IP);
                        bannedUser = 1;
                        break;
                    }
                }
                fclose(fp);
                if (bannedUser) {
                    break;
                }
                // check peer
                // ...
                // find next free ID
                playerID = OnConnect(server);
                if (playerID == 0xFF) {
                    enet_peer_disconnect_now(event.peer, REASON_SERVER_FULL);
                    STATUS("Server full. Kicking player");
                    break;
                }
                server->player[playerID].peer = event.peer;
                event.peer->data              = (void*) ((size_t) playerID);
                server->player[playerID].HP   = 100;
                uint32ToUint8(server, event, playerID);
                printf("INFO: connected %u (%d.%d.%d.%d):%u, id %u\n",
                       event.peer->address.host,
                       server->player[playerID].ip[0],
                       server->player[playerID].ip[1],
                       server->player[playerID].ip[2],
                       server->player[playerID].ip[3],
                       event.peer->address.port,
                       playerID);
                break;
            case ENET_EVENT_TYPE_DISCONNECT:
                playerID = (uint8) ((size_t) event.peer->data);
                SendPlayerLeft(server, playerID);
                Vector3f empty                                       = {0, 0, 0};
                Vector3f forward                                     = {1, 0, 0};
                Vector3f height                                      = {0, 0, 1};
                Vector3f strafe                                      = {0, 1, 0};
                server->player[playerID].state                       = STATE_DISCONNECTED;
                server->player[playerID].queues                      = NULL;
                server->player[playerID].ups                         = 60;
                server->player[playerID].timeSinceLastWU             = get_nanos();
                server->player[playerID].input                       = 0;
                server->player[playerID].movement.eyePos             = empty;
                server->player[playerID].movement.forwardOrientation = forward;
                server->player[playerID].movement.strafeOrientation  = strafe;
                server->player[playerID].movement.heightOrientation  = height;
                server->player[playerID].movement.position           = empty;
                server->player[playerID].movement.velocity           = empty;
                server->player[playerID].airborne                    = 0;
                server->player[playerID].wade                        = 0;
                server->player[playerID].lastclimb                   = 0;
                server->player[playerID].movBackwards                = 0;
                server->player[playerID].movForward                  = 0;
                server->player[playerID].movLeft                     = 0;
                server->player[playerID].movRight                    = 0;
                server->player[playerID].jumping                     = 0;
                server->player[playerID].crouching                   = 0;
                server->player[playerID].sneaking                    = 0;
                server->player[playerID].sprinting                   = 0;
                server->player[playerID].primary_fire                = 0;
                server->player[playerID].secondary_fire              = 0;
                server->player[playerID].canBuild                    = 1;
                server->player[playerID].allowKilling                = 1;
                server->player[playerID].allowTeamKilling            = 0;
                server->player[playerID].muted                       = 0;
                server->player[playerID].toldToMaster                = 0;
                server->player[playerID].sinceLastBaseEnter          = time(NULL);
                server->player[playerID].sinceLastBaseEnterRestock   = time(NULL);
                server->player[playerID].HP                          = 100;
                server->player[playerID].blocks                      = 50;
                server->player[playerID].grenades                    = 3;
                server->player[playerID].isManager                   = 0;
                server->player[playerID].isAdmin                     = 0;
                server->player[playerID].isMod                       = 0;
                server->player[playerID].isGuard                     = 0;
                server->player[playerID].isTrusted                   = 0;
                server->player[playerID].isInvisible                 = 0;
                server->player[playerID].kills                       = 0;
                server->player[playerID].deaths                      = 0;
                memset(server->player[playerID].name, 0, 17);
                server->protocol.numPlayers--;
                if (server->master.enableMasterConnection == 1) {
                    updateMaster(server);
                }
                break;
            case ENET_EVENT_TYPE_RECEIVE:
            {
                DataStream stream = {event.packet->data, event.packet->dataLength, 0};
                playerID          = (uint8) ((size_t) event.peer->data);
                OnPacketReceived(server, playerID, &stream, event);
                enet_packet_destroy(event.packet);
                break;
            }
        }
    }
}

void StartServer(uint16      port,
                 uint32      connections,
                 uint32      channels,
                 uint32      inBandwidth,
                 uint32      outBandwidth,
                 uint8       master,
                 char        mapArray[][64],
                 uint8       mapCount,
                 const char* managerPasswd,
                 const char* adminPasswd,
                 const char* modPasswd,
                 const char* guardPasswd,
                 const char* trustedPasswd,
                 char*       serverName,
                 char*       team1Name,
                 char*       team2Name,
                 uint8*      team1Color,
                 uint8*      team2Color,
                 uint8       gamemode)
{
    timeSinceStart = get_nanos();
    STATUS("Welcome to SpadesX server");
    STATUS("Initializing ENet");

    if (enet_initialize() != 0) {
        ERROR("Failed to initalize ENet");
        exit(EXIT_FAILURE);
    }
    atexit(enet_deinitialize);

    ENetAddress address;
    address.host = ENET_HOST_ANY;
    address.port = port;

    printf("Creating server at port %d\n", port);

    server.host = enet_host_create(&address, connections, channels, inBandwidth, outBandwidth);
    if (server.host == NULL) {
        ERROR("Failed to create server");
        exit(EXIT_FAILURE);
    }

    if (enet_host_compress_with_range_coder(server.host) != 0) {
        WARNING("Compress with range coder failed");
    }

    STATUS("Intializing server");

    ServerInit(&server, connections, mapArray, mapCount, serverName, team1Name, team2Name, team1Color, team2Color, gamemode);
    server.master.enableMasterConnection = master;
    server.managerPasswd                 = managerPasswd;
    server.adminPasswd                   = adminPasswd;
    server.modPasswd                     = modPasswd;
    server.guardPasswd                   = guardPasswd;
    server.trustedPasswd                 = trustedPasswd;

    STATUS("Server started");
    if (server.master.enableMasterConnection == 1) {
        ConnectMaster(&server, port);
    }
    server.master.timeSinceLastSend = time(NULL);
    while (1) {
        calculatePhysics();
        ServerUpdate(&server, 0);
        WorldUpdate();
        keepMasterAlive(&server);
    }

    STATUS("Destroying server");
    enet_host_destroy(server.host);
}
