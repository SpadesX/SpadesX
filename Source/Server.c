// Copyright DarkNeutrino 2021
#include "Server.h"

#include "Commands.h"
#include "Map.h"
#include "Master.h"
#include "Packets.h"
#include "ParseConvert.h"
#include "Ping.h"
#include "Protocol.h"
#include "Structs.h"
#include "Util/Compress.h"
#include "Util/DataStream.h"
#include "Util/Enums.h"
#include "Util/JSONHelpers.h"
#include "Util/Line.h"
#include "Util/Queue.h"
#include "Util/Types.h"
#include "Utlist.h"

#include <Gamemodes.h>
#include <enet/enet.h>
#include <json-c/json.h>
#include <json-c/json_object.h>
#include <json-c/json_util.h>
#include <math.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

Server    server;
pthread_t thread[3];

Server* getServer()
{
    return &server; // Needed when we cant pass Server as argument into function
}

static void freeStringNodes(stringNode* root)
{
    stringNode *el, *tmp;
    DL_FOREACH_SAFE(root, el, tmp)
    {
        free(el->string);
        DL_DELETE(root, el);
        free(el);
    }
}

static unsigned long long get_nanos(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return (unsigned long long) ts.tv_sec * 1000000000L + ts.tv_nsec;
}

static void forPlayers()
{
    for (int playerID = 0; playerID < server.protocol.maxPlayers; ++playerID) {
        if (isPastJoinScreen(&server, playerID)) {
            uint64 timeNow = get_nanos();
            if (server.player[playerID].primary_fire == 1 && server.player[playerID].reloading == 0) {
                if (server.player[playerID].weaponClip > 0) {
                    uint64 milliseconds = 0;
                    switch (server.player[playerID].weapon) {
                        case WEAPON_RIFLE:
                        {
                            milliseconds = 500;
                            break;
                        }
                        case WEAPON_SMG:
                        {
                            milliseconds = 100;
                            break;
                        }
                        case WEAPON_SHOTGUN:
                        {
                            milliseconds = 1000;
                            break;
                        }
                    }
                    if (diffIsOlderThen(
                        timeNow, &server.player[playerID].timers.sinceLastWeaponInput, NANO_IN_MILLI * milliseconds)) {
                        server.player[playerID].toRefill++;
                        server.player[playerID].weaponClip--;
                    }
                }
            } else if (server.player[playerID].primary_fire == 0 && server.player[playerID].reloading == 1) {
                switch (server.player[playerID].weapon) {
                    case WEAPON_RIFLE:
                    case WEAPON_SMG:
                    {
                        if (diffIsOlderThen(
                            timeNow, &server.player[playerID].timers.sinceReloadStart, NANO_IN_MILLI * (uint64) 2500)) {
                            if (server.player[playerID].weaponReserve < server.player[playerID].toRefill) {
                                server.player[playerID].weaponClip += server.player[playerID].weaponReserve;
                                server.player[playerID].toRefill -= server.player[playerID].weaponReserve;
                                server.player[playerID].weaponReserve = 0;
                                server.player[playerID].reloading     = 0;
                                SendWeaponReload(&server, playerID, 0, 0, 0);
                                break;
                            }
                            server.player[playerID].weaponClip += server.player[playerID].toRefill;
                            server.player[playerID].weaponReserve -= server.player[playerID].toRefill;
                            server.player[playerID].reloading = 0;
                            server.player[playerID].toRefill  = 0;
                            SendWeaponReload(&server, playerID, 0, 0, 0);
                        }
                        break;
                    }
                    case WEAPON_SHOTGUN:
                    {
                        if (diffIsOlderThen(
                            timeNow, &server.player[playerID].timers.sinceReloadStart, NANO_IN_MILLI * (uint64) 500)) {
                            if (server.player[playerID].weaponReserve == 0 || server.player[playerID].toRefill == 0) {
                                server.player[playerID].reloading = 0;
                                break;
                            }
                            server.player[playerID].weaponClip++;
                            server.player[playerID].weaponReserve--;
                            server.player[playerID].toRefill--;
                            if (server.player[playerID].toRefill == 0) {
                                server.player[playerID].reloading = 0;
                            }
                            SendWeaponReload(&server, playerID, 0, 0, 0);
                        }
                        break;
                    }
                }
            }
            if (diffIsOlderThen(timeNow,
                                &server.player[playerID].timers.sincePeriodicMessage,
                                (uint64) (server.periodicDelays[server.player[playerID].periodicDelayIndex] * 60) *
                                NANO_IN_SECOND))
            {
                stringNode* message;
                DL_FOREACH(server.periodicMessages, message)
                {
                    sendServerNotice(&server, playerID, message->string);
                }
                server.player[playerID].periodicDelayIndex = fmin(server.player[playerID].periodicDelayIndex + 1, 4);
            }
        } else if (isPastStateData(&server, playerID)) {
        }
    }
}

static void* calculatePhysics()
{
    server.globalTimers.updateTime = get_nanos();
    if (server.globalTimers.updateTime - server.globalTimers.lastUpdateTime >= (1000000000 / 60)) {
        updateMovementAndGrenades(&server);
        server.globalTimers.lastUpdateTime = get_nanos();
    }
    return 0;
}

static void ServerInit(Server*     server,
                       uint32      connections,
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

    char  vxlMap[64];
    uint8 index;
    if (reset == 0) {
        srand(time(0));
    }
    index                  = rand() % server->map.mapCount;
    server->map.currentMap = server->map.mapList;
    for (int i = 0; i <= index; ++i) {
        if (server->map.currentMap->next != NULL) {
            server->map.currentMap = server->map.currentMap->next;
        } else {
            break; // Safety if we by some magical reason go beyond the list
        }
    }
    snprintf(
    server->mapName, fmin(strlen(server->map.currentMap->string) + 1, 20), "%s", server->map.currentMap->string);
    LOG_STATUS("Selecting %s as map", server->mapName);

    snprintf(vxlMap, 64, "%s.vxl", server->map.currentMap->string);
    LoadMap(server, vxlMap);

    LOG_STATUS("Loading spawn ranges from map file");
    char mapConfig[64];
    snprintf(mapConfig, 64, "%s.json", server->map.currentMap->string);

    struct json_object* parsed_map_json;
    parsed_map_json = json_object_from_file(mapConfig);

    int         team1Start[2];
    int         team2Start[2];
    int         team1End[2];
    int         team2End[2];
    const char* author;

    READ_INT_ARR_FROM_JSON(parsed_map_json, team1Start, team1_start, "team1 start", ((int[]){0, 0}), 2, 0)
    READ_INT_ARR_FROM_JSON(parsed_map_json, team1End, team1_end, "team1 end", ((int[]){10, 10}), 2, 0)
    READ_INT_ARR_FROM_JSON(parsed_map_json, team2Start, team2_start, "team2 start", ((int[]){20, 20}), 2, 0)
    READ_INT_ARR_FROM_JSON(parsed_map_json, team2End, team2_end, "team2 end", ((int[]){30, 30}), 2, 0)
    READ_STR_FROM_JSON(parsed_map_json, author, author, "author", "Unknown", 0)
    (void) author;

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
    server->protocol.spawns[0].to.x   = team1End[0];
    server->protocol.spawns[0].to.y   = team1End[1];

    server->protocol.spawns[1].from.x = team2Start[0];
    server->protocol.spawns[1].from.y = team2Start[1];
    server->protocol.spawns[1].to.x   = team2End[0];
    server->protocol.spawns[1].to.y   = team2End[1];

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
    LOG_STATUS("Sending state");
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
            server->player[playerID].HP        = 100;
            server->player[playerID].grenades  = 3;
            server->player[playerID].blocks    = 50;
            server->player[playerID].alive     = 1;
            server->player[playerID].reloading = 0;
            server->player[playerID].toRefill  = 0;
            SetPlayerRespawnPoint(server, playerID);
            SendRespawn(server, playerID);
            LOG_INFO("Player %d spawning at: %f %f %f",
                     playerID,
                     server->player[playerID].movement.position.x,
                     server->player[playerID].movement.position.y,
                     server->player[playerID].movement.position.z);
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
            uint64 time = get_nanos();
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
                LOG_STATUS("Event of type none received. Ignoring");
                break;
            case ENET_EVENT_TYPE_CONNECT:
                if (event.data != VERSION_0_75) {
                    enet_peer_disconnect_now(event.peer, REASON_WRONG_PROTOCOL_VERSION);
                    break;
                }

                struct json_object* root = json_object_from_file("Bans.json");
                if (root == NULL) {
                    FILE* fp;
                    fp = fopen("Bans.json", "w+");
                    fclose(fp);
                    root = json_object_new_object();
                    json_object_object_add(root, "Bans", json_object_new_array());
                    json_object_to_file("Bans.json", root);
                }
                IPStruct hostIP;
                hostIP.Union.ip32 = event.peer->address.host;
                struct json_object* array;
                json_object_object_get_ex(root, "Bans", &array);
                int count = json_object_array_length(array);
                for (int i = 0; i < count; ++i) {
                    struct json_object* objectAtIndex = json_object_array_get_idx(array, i);
                    const char*         IP;
                    READ_STR_FROM_JSON(objectAtIndex, IP, IP, "IP", "0.0.0.0", 0);
                    IPStruct ipStruct;
                    if (formatStringToIP((char*)IP, &ipStruct)) {
                        if (ipStruct.Union.ip32 == hostIP.Union.ip32)
                        {
                            const char* nameOfPlayer;
                            const char* reason;
                            READ_STR_FROM_JSON(objectAtIndex, nameOfPlayer, Name, "Name", "Deuce", 0);
                            READ_STR_FROM_JSON(objectAtIndex, reason, Reason, "Reason", "None", 0);
                            enet_peer_disconnect(event.peer, REASON_BANNED);

                            LOG_WARNING("Banned user %s tried to join with IP: %hhu.%hhu.%hhu.%hhu Banned for: %s",
                                        nameOfPlayer,
                                        hostIP.Union.ip[0],
                                        hostIP.Union.ip[1],
                                        hostIP.Union.ip[2],
                                        hostIP.Union.ip[3],
                                        reason);
                            bannedUser = 1;
                            break;
                        }
                    }
                }
                json_object_put(root);

                if (bannedUser) {
                    break;
                }
                // check peer
                // ...
                // find next free ID
                playerID = OnConnect(server);
                if (playerID == 0xFF) {
                    enet_peer_disconnect_now(event.peer, REASON_SERVER_FULL);
                    LOG_WARNING("Server full. Kicking player");
                    break;
                }
                server->player[playerID].peer                = event.peer;
                event.peer->data                             = (void*) ((size_t) playerID);
                server->player[playerID].HP                  = 100;
                server->player[playerID].ipStruct.Union.ip32 = event.peer->address.host;
                LOG_STATUS("Connected %u (%d.%d.%d.%d):%u, id %u",
                           event.peer->address.host,
                           server->player[playerID].ipStruct.Union.ip[0],
                           server->player[playerID].ipStruct.Union.ip[1],
                           server->player[playerID].ipStruct.Union.ip[2],
                           server->player[playerID].ipStruct.Union.ip[3],
                           event.peer->address.port,
                           playerID);
                server->player[playerID].timers.sincePeriodicMessage = get_nanos();
                server->player[playerID].currentPeriodicMessage      = server->periodicMessages;
                server->player[playerID].periodicDelayIndex          = 0;
                break;
            case ENET_EVENT_TYPE_DISCONNECT:
                playerID = (uint8) ((size_t) event.peer->data);
                SendIntelDrop(server, playerID);
                SendPlayerLeft(server, playerID);
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

void StopServer(int signal)
{
    (void) signal; // To prevent a warning about unused variable
    server.running = 0;
}

void StartServer(uint16      port,
                 uint32      connections,
                 uint32      channels,
                 uint32      inBandwidth,
                 uint32      outBandwidth,
                 uint8       master,
                 stringNode* mapList,
                 uint8       mapCount,
                 stringNode* welcomeMessageList,
                 uint8       welcomeMessageListLen,
                 stringNode* periodicMessageList,
                 uint8       periodicMessageListLen,
                 uint8*      periodicDelays,
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
    signal(SIGINT, StopServer);

    server.globalTimers.timeSinceStart = get_nanos();
    LOG_STATUS("Welcome to SpadesX server");
    LOG_STATUS("Initializing ENet");

    if (enet_initialize() != 0) {
        LOG_ERROR("Failed to initalize ENet");
        exit(EXIT_FAILURE);
    }
    atexit(enet_deinitialize);

    ENetAddress address;
    address.host = ENET_HOST_ANY;
    address.port = port;

    LOG_STATUS("Creating server at port %d", port);

    server.host = enet_host_create(&address, connections, channels, inBandwidth, outBandwidth);
    if (server.host == NULL) {
        LOG_ERROR("Failed to create server");
        exit(EXIT_FAILURE);
    }

    if (enet_host_compress_with_range_coder(server.host) != 0) {
        LOG_WARNING("Compress with range coder failed");
    }

    server.port = port;

    server.host->intercept = &rawUdpInterceptCallback;

    LOG_STATUS("Intializing server");
    server.running              = 1;
    server.map.mapList          = mapList;
    server.map.mapCount         = mapCount;
    server.welcomeMessages      = welcomeMessageList;
    server.welcomeMessageCount  = welcomeMessageListLen;
    server.periodicMessages     = periodicMessageList;
    server.periodicMessageCount = periodicMessageListLen;
    server.periodicDelays       = periodicDelays;
    ServerInit(&server, connections, serverName, team1Name, team2Name, team1Color, team2Color, gamemode, 0);

    populateCommands(&server);

    server.master.enableMasterConnection = master;
    server.managerPasswd                 = managerPasswd;
    server.adminPasswd                   = adminPasswd;
    server.modPasswd                     = modPasswd;
    server.guardPasswd                   = guardPasswd;
    server.trustedPasswd                 = trustedPasswd;

    if (server.running) {
        LOG_STATUS("Server started");
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

    freeCommands(&server);

    freeStringNodes(server.welcomeMessages);
    freeStringNodes(server.map.mapList);
    freeStringNodes(server.periodicMessages);

    LOG_STATUS("Exiting");
    enet_host_destroy(server.host);
}
