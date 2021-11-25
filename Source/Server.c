// Copyright DarkNeutrino 2021
#include "Server.h"

#include "Conversion.h"
#include "Enums.h"
#include "Map.h"
#include "Master.h"
#include "Packets.h"
#include "Protocol.h"
#include "Structs.h"

#include <Compress.h>
#include <DataStream.h>
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
                       const char* serverName,
                       const char* team1Name,
                       const char* team2Name,
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

    char vxlMap[64];
    srand(time(0));
    uint8 index          = rand() % mapCount;
    server->map.mapIndex = index;
    memcpy(server->mapName, mapArray[index], strlen(mapArray[index]) + 1);
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
        server->player[i].state                       = STATE_DISCONNECTED;
        server->player[i].queues                      = NULL;
        server->player[i].ups                         = 60;
        server->player[i].timers.timeSinceLastWU      = get_nanos();
        server->player[i].input                       = 0;
        server->player[i].movement.eyePos             = empty;
        server->player[i].movement.forwardOrientation = forward;
        server->player[i].movement.strafeOrientation  = strafe;
        server->player[i].movement.heightOrientation  = height;
        server->player[i].movement.position           = empty;
        server->player[i].movement.velocity           = empty;
        PermLevel roleList[5] = {{"manager", &server->managerPasswd, &server->player[i].isManager},
                                 {"admin", &server->adminPasswd, &server->player[i].isAdmin},
                                 {"mod", &server->modPasswd, &server->player[i].isMod},
                                 {"guard", &server->guardPasswd, &server->player[i].isGuard},
                                 {"trusted", &server->trustedPasswd, &server->player[i].isTrusted}};
        for (unsigned long x = 0; x < sizeof(roleList) / sizeof(PermLevel); ++x) {
            server->player[i].roleList[x] = roleList[x];
        }
        server->player[i].airborne                         = 0;
        server->player[i].wade                             = 0;
        server->player[i].lastclimb                        = 0;
        server->player[i].movBackwards                     = 0;
        server->player[i].movForward                       = 0;
        server->player[i].movLeft                          = 0;
        server->player[i].movRight                         = 0;
        server->player[i].jumping                          = 0;
        server->player[i].crouching                        = 0;
        server->player[i].sneaking                         = 0;
        server->player[i].sprinting                        = 0;
        server->player[i].primary_fire                     = 0;
        server->player[i].secondary_fire                   = 0;
        server->player[i].canBuild                         = 1;
        server->player[i].allowKilling                     = 1;
        server->player[i].allowTeamKilling                 = 0;
        server->player[i].muted                            = 0;
        server->player[i].toldToMaster                     = 0;
        server->player[i].timers.sinceLastBaseEnter        = 0;
        server->player[i].timers.sinceLastBaseEnterRestock = 0;
        server->player[i].timers.sinceLast3BlockDest       = 0;
        server->player[i].timers.sinceLastBlockDest        = 0;
        server->player[i].timers.sinceLastBlockPlac        = 0;
        server->player[i].timers.sinceLastGrenadeThrown    = 0;
        server->player[i].timers.sinceLastShot             = 0;
        server->player[i].timers.timeSinceLastWU           = 0;
        server->player[i].timers.sinceLastWeaponInput      = 0;
        server->player[i].HP                               = 100;
        server->player[i].blocks                           = 50;
        server->player[i].grenades                         = 3;
        server->player[i].hasIntel                         = 0;
        server->player[i].isManager                        = 0;
        server->player[i].isAdmin                          = 0;
        server->player[i].isMod                            = 0;
        server->player[i].isGuard                          = 0;
        server->player[i].isTrusted                        = 0;
        server->player[i].isInvisible                      = 0;
        server->player[i].kills                            = 0;
        server->player[i].deaths                           = 0;
        memset(server->player[i].name, 0, 17);
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
}

void ServerReset(Server* server)
{
    updateTime = lastUpdateTime = get_nanos();

    server->map.compressedMap   = NULL;
    server->map.compressedSize  = 0;
    server->protocol.inputFlags = 0;

    char  vxlMap[64];
    uint8 index          = rand() % server->map.mapCount;
    server->map.mapIndex = index;
    memcpy(server->mapName, server->map.mapArray[index], strlen(server->map.mapArray[index]) + 1);
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
        server->player[i].ups                              = 60;
        server->player[i].timers.timeSinceLastWU           = get_nanos();
        server->player[i].input                            = 0;
        server->player[i].movement.eyePos                  = empty;
        server->player[i].movement.forwardOrientation      = forward;
        server->player[i].movement.strafeOrientation       = strafe;
        server->player[i].movement.heightOrientation       = height;
        server->player[i].movement.position                = empty;
        server->player[i].movement.velocity                = empty;
        server->player[i].airborne                         = 0;
        server->player[i].wade                             = 0;
        server->player[i].lastclimb                        = 0;
        server->player[i].movBackwards                     = 0;
        server->player[i].movForward                       = 0;
        server->player[i].movLeft                          = 0;
        server->player[i].movRight                         = 0;
        server->player[i].jumping                          = 0;
        server->player[i].crouching                        = 0;
        server->player[i].sneaking                         = 0;
        server->player[i].sprinting                        = 0;
        server->player[i].primary_fire                     = 0;
        server->player[i].secondary_fire                   = 0;
        server->player[i].canBuild                         = 1;
        server->player[i].allowKilling                     = 1;
        server->player[i].allowTeamKilling                 = 0;
        server->player[i].muted                            = 0;
        server->player[i].toldToMaster                     = 0;
        server->player[i].timers.sinceLastBaseEnter        = 0;
        server->player[i].timers.sinceLastBaseEnterRestock = 0;
        server->player[i].timers.sinceLast3BlockDest       = 0;
        server->player[i].timers.sinceLastBlockDest        = 0;
        server->player[i].timers.sinceLastBlockPlac        = 0;
        server->player[i].timers.sinceLastGrenadeThrown    = 0;
        server->player[i].timers.sinceLastShot             = 0;
        server->player[i].timers.timeSinceLastWU           = 0;
        server->player[i].timers.sinceLastWeaponInput      = 0;
        server->player[i].HP                               = 100;
        server->player[i].blocks                           = 50;
        server->player[i].grenades                         = 3;
        server->player[i].hasIntel                         = 0;
        server->player[i].isInvisible                      = 0;
        server->player[i].kills                            = 0;
        server->player[i].deaths                           = 0;
        memset(server->player[i].name, 0, 17);
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
                uint8 team;
                if (server->player[playerID].team == 0) {
                    team = 1;
                } else {
                    team = 0;
                }
                Vector3f empty                                            = {0, 0, 0};
                Vector3f forward                                          = {1, 0, 0};
                Vector3f height                                           = {0, 0, 1};
                Vector3f strafe                                           = {0, 1, 0};
                server->player[playerID].state                            = STATE_DISCONNECTED;
                server->player[playerID].queues                           = NULL;
                server->player[playerID].ups                              = 60;
                server->player[playerID].timers.timeSinceLastWU           = get_nanos();
                server->player[playerID].input                            = 0;
                server->player[playerID].movement.eyePos                  = empty;
                server->player[playerID].movement.forwardOrientation      = forward;
                server->player[playerID].movement.strafeOrientation       = strafe;
                server->player[playerID].movement.heightOrientation       = height;
                server->player[playerID].movement.position                = empty;
                server->player[playerID].movement.velocity                = empty;
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
                server->player[playerID].isManager                        = 0;
                server->player[playerID].isAdmin                          = 0;
                server->player[playerID].isMod                            = 0;
                server->player[playerID].isGuard                          = 0;
                server->player[playerID].isTrusted                        = 0;
                server->player[playerID].isInvisible                      = 0;
                server->player[playerID].kills                            = 0;
                server->player[playerID].deaths                           = 0;
                server->protocol.ctf.intelHeld[team]                      = 0;
                memset(server->player[playerID].name, 0, 17);
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
    server.running = 1;
    ServerInit(
    &server, connections, mapArray, mapCount, serverName, team1Name, team2Name, team1Color, team2Color, gamemode);
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
