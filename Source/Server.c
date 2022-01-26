// Copyright DarkNeutrino 2021
#include "Server.h"

#include "Conversion.h"
#include "Map.h"
#include "Master.h"
#include "Packets.h"
#include "Protocol.h"
#include "Structs.h"

#include <Compress.h>
#include <DataStream.h>
#include <Enums.h>
#include <Gamemodes.h>
#include <Line.h>
#include <Queue.h>
#include <Types.h>
#include <enet/enet.h>
#include <json-c/json.h>
#include <json-c/json_object.h>
#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

Server    server;
pthread_t thread[3];

static unsigned long long get_nanos(void)
{
    struct timespec ts;
    timespec_get(&ts, TIME_UTC);
    return (unsigned long long) ts.tv_sec * 1000000000L + ts.tv_nsec;
}

static void forPlayers()
{
    for (int playerID = 0; playerID < server.protocol.maxPlayers; ++playerID) {
        if (isPastJoinScreen(&server, playerID)) {
            time_t timeNow = get_nanos();
            if (server.player[playerID].primary_fire) {
                if (server.player[playerID].weaponClip > 0) {
                    switch (server.player[playerID].weapon) {
                        case WEAPON_RIFLE:
                        {
                            if (diffIsOlderThen(
                                timeNow, &server.player[playerID].timers.sinceLastWeaponInput, NANO_IN_MILLI * 500)) {
                                server.player[playerID].weaponClip--;
                            }
                            break;
                        }
                        case WEAPON_SMG:
                        {
                            if (diffIsOlderThen(
                                timeNow, &server.player[playerID].timers.sinceLastWeaponInput, NANO_IN_MILLI * 110)) {
                                server.player[playerID].weaponClip--;
                            }
                            break;
                        }
                        case WEAPON_SHOTGUN:
                        {
                            if (diffIsOlderThen(
                                timeNow, &server.player[playerID].timers.sinceLastWeaponInput, NANO_IN_MILLI * 1000)) {
                                server.player[playerID].weaponClip--;
                            }
                            break;
                        }
                    }
                }
            }
        }
        if (isPastStateData(&server, playerID)) {
        }
    }
}

static void* calculatePhysics()
{
    server.globalTimers.updateTime = get_nanos();
    if (server.globalTimers.updateTime - server.globalTimers.lastUpdateTime >= (1000000000 / 120)) {
        updateMovementAndGrenades(&server);
        server.globalTimers.lastUpdateTime = get_nanos();
    }
    return 0;
}

static void ServerInit(Server*     server,
                       uint32      connections,
                       char        mapArray[][64],
                       uint8       mapCount,
                       const char* serverName,
                       const char* team1Name,
                       const char* team2Name,
                       uint8*      team1Color,
                       uint8*      team2Color,
                       uint8       gamemode,
                       uint8       reset)
{
    server->globalTimers.updateTime = server->globalTimers.lastUpdateTime = get_nanos();
    if (reset == 0) {
        server->protocol.numPlayers = 0;
        server->protocol.maxPlayers = (connections <= 32) ? connections : 32;
    }

    server->map.compressedMap   = NULL;
    server->map.compressedSize  = 0;
    server->protocol.inputFlags = 0;
    if (reset == 0) {
        for (int i = 0; i < mapCount; ++i) {
            memcpy(server->map.mapArray, mapArray[i], strlen(mapArray[i]));
        }
        server->map.mapCount = mapCount;
    }

    char  vxlMap[64];
    uint8 index;
    if (reset == 0) {
        srand(time(0));
        index = rand() % mapCount;
    } else {
        index = rand() % server->map.mapCount;
    }
    server->map.mapIndex = index;
    if (reset == 0) {
        memcpy(server->mapName, mapArray[index], strlen(mapArray[index]) + 1);
    } else {
        memcpy(server->mapName, server->map.mapArray[index], strlen(server->map.mapArray[index]) + 1);
    }
    printf("STATUS: Selecting %s as map\n", server->mapName);

    snprintf(vxlMap, 64, "%s.vxl", server->mapName);
    LoadMap(server, vxlMap);

    STATUS("Loading spawn ranges from map file");
    char mapConfig[64];
    snprintf(mapConfig, 64, "%s.json", server->mapName);

    struct json_object* parsed_map_json;
    parsed_map_json = json_object_from_file(mapConfig);

    struct json_object* team1StartInConfig;
    struct json_object* team1EndInConfig;
    struct json_object* team2StartInConfig;
    struct json_object* team2EndInConfig;
    struct json_object* authorInConfig;
    int                 team1Start[3];
    int                 team2Start[3];
    int                 team1End[3];
    int                 team2End[3];

    if (json_object_object_get_ex(parsed_map_json, "team1_start", &team1StartInConfig) == 0) {
        ERROR("Failed to find team1 start in map config");
        server->running = 0;
        return;
    }
    if (json_object_object_get_ex(parsed_map_json, "team1_end", &team1EndInConfig) == 0) {
        ERROR("Failed to find team1 end in map config");
        server->running = 0;
        return;
    }
    if (json_object_object_get_ex(parsed_map_json, "team2_start", &team2StartInConfig) == 0) {
        ERROR("Failed to find team2 start in map config");
        server->running = 0;
        return;
    }
    if (json_object_object_get_ex(parsed_map_json, "team2_end", &team2EndInConfig) == 0) {
        ERROR("Failed to find team2 start in map config");
        server->running = 0;
        return;
    }
    if (json_object_object_get_ex(parsed_map_json, "author", &authorInConfig) == 0) {
        ERROR("Failed to find author in map config");
        server->running = 0;
        return;
    }

    for (uint8 i = 0; i < 2; ++i) {
        team1Start[i] = json_object_get_int(json_object_array_get_idx(team1StartInConfig, i));
        team2Start[i] = json_object_get_int(json_object_array_get_idx(team2StartInConfig, i));
        team1End[i]   = json_object_get_int(json_object_array_get_idx(team1EndInConfig, i));
        team2End[i]   = json_object_get_int(json_object_array_get_idx(team2EndInConfig, i));
    }

    json_object_put(parsed_map_json);

    Vector3f empty   = {0, 0, 0};
    Vector3f forward = {1, 0, 0};
    Vector3f height  = {0, 0, 1};
    Vector3f strafe  = {0, 1, 0};
    for (uint32 i = 0; i < server->protocol.maxPlayers; ++i) {
        initPlayer(server, i, reset, 0, empty, forward, strafe, height);
    }

    server->globalAB = 1;
    server->globalAK = 1;

    server->protocol.spawns[0].from.x = team1Start[0];
    server->protocol.spawns[0].from.y = team1Start[1];
    server->protocol.spawns[0].from.z = team1Start[2];
    server->protocol.spawns[0].to.x   = team1End[0];
    server->protocol.spawns[0].to.y   = team1End[1];
    server->protocol.spawns[0].to.z   = team1End[2];

    server->protocol.spawns[1].from.x = team2Start[0];
    server->protocol.spawns[1].from.y = team2Start[1];
    server->protocol.spawns[1].from.z = team2Start[2];
    server->protocol.spawns[1].to.x   = team2End[0];
    server->protocol.spawns[1].to.y   = team2End[1];
    server->protocol.spawns[1].to.z   = team2End[2];

    server->protocol.colorFog.colorArray[0] = 0x80;
    server->protocol.colorFog.colorArray[1] = 0xE8;
    server->protocol.colorFog.colorArray[2] = 0xFF;

    server->protocol.colorTeam[0].colorArray[B_CHANNEL] = team1Color[B_CHANNEL];
    server->protocol.colorTeam[0].colorArray[G_CHANNEL] = team1Color[G_CHANNEL];
    server->protocol.colorTeam[0].colorArray[R_CHANNEL] = team1Color[R_CHANNEL];

    server->protocol.colorTeam[1].colorArray[B_CHANNEL] = team2Color[B_CHANNEL];
    server->protocol.colorTeam[1].colorArray[G_CHANNEL] = team2Color[G_CHANNEL];
    server->protocol.colorTeam[1].colorArray[R_CHANNEL] = team2Color[R_CHANNEL];

    memcpy(server->protocol.nameTeam[0], team1Name, strlen(team1Name));
    memcpy(server->protocol.nameTeam[1], team2Name, strlen(team2Name));
    server->protocol.nameTeam[0][strlen(team1Name)] = '\0';
    server->protocol.nameTeam[1][strlen(team2Name)] = '\0';

    memcpy(server->serverName, serverName, strlen(serverName));
    server->serverName[strlen(serverName)] = '\0';
    initGameMode(server, gamemode);
}

void ServerReset(Server* server)
{
    ServerInit(server,
               server->protocol.maxPlayers,
               server->map.mapArray,
               server->map.mapCount,
               server->serverName,
               server->protocol.nameTeam[0],
               server->protocol.nameTeam[1],
               &server->protocol.colorTeam->colorArray[0],
               &server->protocol.colorTeam->colorArray[1],
               server->protocol.currentGameMode,
               1);
}

static void SendJoiningData(Server* server, uint8 playerID)
{
    STATUS("sending state");
    for (uint8 i = 0; i < server->protocol.maxPlayers; ++i) {
        if (i != playerID && isPastStateData(server, i)) {
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
            if (time(NULL) - server->player[playerID].timers.startOfRespawnWait >= server->player[playerID].respawnTime)
            {
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
            time_t time = get_nanos();
            if (time - server.player[playerID].timers.timeSinceLastWU >= (NANO_IN_SECOND / server.player[playerID].ups))
            {
                SendWorldUpdate(&server, playerID);
                server.player[playerID].timers.timeSinceLastWU = get_nanos();
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
                    ERROR(
                    "BanList.txt could not be opened for checking ban. PLEASE FIX THIS NOW BY CREATING THIS FILE!!!!");
                    server->running = 0;
                    return;
                }
                uint8  IP[4];
                char   nameOfPlayer[20];
                uint8* hostIP;
                hostIP = uint32ToUint8(event.peer->address.host);
                while (fscanf(fp, "%hhu.%hhu.%hhu.%hhu, %17[^,],", &IP[0], &IP[1], &IP[2], &IP[3], nameOfPlayer) != EOF)
                {
                    if (IP[0] == hostIP[0] && IP[1] == hostIP[1] && IP[2] == hostIP[2] && IP[3] == hostIP[3]) {
                        enet_peer_disconnect(event.peer, REASON_BANNED);
                        printf("WARNING: Banned user %s tried to join. IP: %hhu.%hhu.%hhu.%hhu\n",
                               nameOfPlayer,
                               IP[0],
                               IP[1],
                               IP[2],
                               IP[3]);
                        bannedUser = 1;
                        break;
                    }
                }
                fclose(fp);
                free(hostIP);
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
                server->player[playerID].peer         = event.peer;
                event.peer->data                      = (void*) ((size_t) playerID);
                server->player[playerID].HP           = 100;
                server->player[playerID].ipUnion.ip32 = event.peer->address.host;
                printf("INFO: connected %u (%d.%d.%d.%d):%u, id %u\n",
                       event.peer->address.host,
                       server->player[playerID].ipUnion.ip[0],
                       server->player[playerID].ipUnion.ip[1],
                       server->player[playerID].ipUnion.ip[2],
                       server->player[playerID].ipUnion.ip[3],
                       event.peer->address.port,
                       playerID);
                break;
            case ENET_EVENT_TYPE_DISCONNECT:
                playerID = (uint8) ((size_t) event.peer->data);
                SendPlayerLeft(server, playerID);
                SendIntelDrop(server, playerID);
                Vector3f empty   = {0, 0, 0};
                Vector3f forward = {1, 0, 0};
                Vector3f height  = {0, 0, 1};
                Vector3f strafe  = {0, 1, 0};
                initPlayer(server, playerID, 0, 1, empty, forward, strafe, height);
                server->protocol.numPlayers--;
                server->protocol.teamUserCount[server->player[playerID].team]--;
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
                 const char* serverName,
                 const char* team1Name,
                 const char* team2Name,
                 uint8*      team1Color,
                 uint8*      team2Color,
                 uint8       gamemode)
{
    server.globalTimers.timeSinceStart = get_nanos();
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

    server.port = port;

    STATUS("Intializing server");
    server.running = 1;
    ServerInit(
    &server, connections, mapArray, mapCount, serverName, team1Name, team2Name, team1Color, team2Color, gamemode, 0);
    server.master.enableMasterConnection = master;
    server.managerPasswd                 = managerPasswd;
    server.adminPasswd                   = adminPasswd;
    server.modPasswd                     = modPasswd;
    server.guardPasswd                   = guardPasswd;
    server.trustedPasswd                 = trustedPasswd;

    if (server.running) {
        STATUS("Server started");
    }
    if (server.master.enableMasterConnection == 1) {
        ConnectMaster(&server, port);
    }
    server.master.timeSinceLastSend = time(NULL);
    while (server.running) {
        calculatePhysics();
        ServerUpdate(&server, 0);
        WorldUpdate();
        keepMasterAlive(&server);
        forPlayers();
        sleep(0);
    }
    while (server.map.compressedMap) {
        server.map.compressedMap = Pop(server.map.compressedMap);
    }
    free(server.map.compressedMap);

    STATUS("Exiting");
    enet_host_destroy(server.host);
}
