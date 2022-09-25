// Copyright DarkNeutrino 2021
#include "Server.h"

#include "../Extern/libmapvxl/libmapvxl.h"
#include "Commands.h"
#include "Gamemodes.h"
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
#include "Util/Log.h"
#include "Util/Queue.h"
#include "Util/Types.h"
#include "Util/Utlist.h"

#include <enet/enet.h>
#include <errno.h>
#include <json-c/json.h>
#include <json-c/json_object.h>
#include <json-c/json_types.h>
#include <json-c/json_util.h>
#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include <readline/history.h>
#include <readline/readline.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

Server          server;
pthread_mutex_t serverLock;

volatile int ctrlc = 0;

Server* getServer(void)
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

static void forPlayers(void)
{
    for (int playerID = 0; playerID < server.protocol.maxPlayers; ++playerID) {
        if (isPastJoinScreen(&server, playerID)) {
            uint64 timeNow = getNanos();
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
                        timeNow, &server.player[playerID].timers.sinceLastWeaponInput, NANO_IN_MILLI * milliseconds))
                    {
                        server.player[playerID].weaponClip--;
                    }
                }
            } else if (server.player[playerID].primary_fire == 0 && server.player[playerID].reloading == 1) {
                switch (server.player[playerID].weapon) {
                    case WEAPON_RIFLE:
                    case WEAPON_SMG:
                    {
                        if (diffIsOlderThen(
                            timeNow, &server.player[playerID].timers.sinceReloadStart, NANO_IN_MILLI * (uint64) 2500))
                        {
                            uint8 defaultAmmo = RIFLE_DEFAULT_CLIP;
                            if (server.player[playerID].weapon == WEAPON_SMG) {
                                defaultAmmo = SMG_DEFAULT_CLIP;
                            }
                            double newReserve = fmax(0,
                                                     server.player[playerID].weaponReserve -
                                                     (defaultAmmo - server.player[playerID].weaponClip));
                            server.player[playerID].weaponClip += server.player[playerID].weaponReserve - newReserve;
                            server.player[playerID].weaponReserve = newReserve;
                            server.player[playerID].reloading     = 0;
                            sendWeaponReload(&server, playerID, 0, 0, 0);
                        }
                        break;
                    }
                    case WEAPON_SHOTGUN:
                    {
                        if (diffIsOlderThen(
                            timeNow, &server.player[playerID].timers.sinceReloadStart, NANO_IN_MILLI * (uint64) 500))
                        {
                            if (server.player[playerID].weaponReserve == 0 ||
                                server.player[playerID].weaponClip == SHOTGUN_DEFAULT_CLIP)
                            {
                                server.player[playerID].reloading = 0;
                                break;
                            }
                            server.player[playerID].weaponClip++;
                            server.player[playerID].weaponReserve--;
                            if (server.player[playerID].weaponClip == SHOTGUN_DEFAULT_CLIP) {
                                server.player[playerID].reloading = 0;
                            }
                            sendWeaponReload(&server, playerID, 0, 0, 0);
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
                    sendServerNotice(&server, playerID, 0, message->string);
                }
                server.player[playerID].periodicDelayIndex = fmin(server.player[playerID].periodicDelayIndex + 1, 4);
            }
        } else if (server.player[playerID].state == STATE_PICK_SCREEN) {
            blockNode *node, *tmp;
            LL_FOREACH_SAFE(server.player[playerID].blockBuffer, node, tmp)
            {
                if (node->type == 10) {
                    sendSetColorToPlayer(&server,
                                         node->senderID,
                                         playerID,
                                         node->color.colorArray[0],
                                         node->color.colorArray[1],
                                         node->color.colorArray[2]);
                    sendBlockLineToPlayer(&server, node->senderID, playerID, node->position, node->positionEnd);
                    sendSetColorToPlayer(&server,
                                         node->senderID,
                                         playerID,
                                         server.player[node->senderID].color.colorArray[0],
                                         server.player[node->senderID].color.colorArray[1],
                                         server.player[node->senderID].color.colorArray[2]);
                } else {
                    sendSetColorToPlayer(&server,
                                         node->senderID,
                                         playerID,
                                         node->color.colorArray[0],
                                         node->color.colorArray[1],
                                         node->color.colorArray[2]);
                    sendBlockActionToPlayer(&server,
                                            node->senderID,
                                            playerID,
                                            node->type,
                                            node->position.x,
                                            node->position.y,
                                            node->position.z);
                    sendSetColorToPlayer(&server,
                                         node->senderID,
                                         playerID,
                                         server.player[node->senderID].color.colorArray[0],
                                         server.player[node->senderID].color.colorArray[1],
                                         server.player[node->senderID].color.colorArray[2]);
                }
                LL_DELETE(server.player[playerID].blockBuffer, node);
                free(node);
            }
        } else if (isPastStateData(&server, playerID)) {
        }
    }
}

static void* calculatePhysics(void)
{
    server.globalTimers.updateTime = getNanos();
    if (server.globalTimers.updateTime - server.globalTimers.lastUpdateTime >= (1000000000 / 60)) {
        updateMovementAndGrenades(&server);
        server.globalTimers.lastUpdateTime = getNanos();
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
    server->globalTimers.updateTime = server->globalTimers.lastUpdateTime = getNanos();
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
        srand(time(NULL));
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

    LOG_STATUS("Loading spawn ranges from map file");
    char mapConfig[64];
    snprintf(mapConfig, 64, "%s.json", server->map.currentMap->string);

    struct json_object* parsed_map_json;
    parsed_map_json = json_object_from_file(mapConfig);

    int         team1Start[2];
    int         team2Start[2];
    int         team1End[2];
    int         team2End[2];
    int         mapSize[3];
    const char* author;

    READ_INT_ARR_FROM_JSON(parsed_map_json, team1Start, team1_start, "team1 start", ((int[]){0, 0}), 2, 0)
    READ_INT_ARR_FROM_JSON(parsed_map_json, team1End, team1_end, "team1 end", ((int[]){10, 10}), 2, 0)
    READ_INT_ARR_FROM_JSON(parsed_map_json, team2Start, team2_start, "team2 start", ((int[]){20, 20}), 2, 0)
    READ_INT_ARR_FROM_JSON(parsed_map_json, team2End, team2_end, "team2 end", ((int[]){30, 30}), 2, 0)
    READ_INT_ARR_FROM_JSON(parsed_map_json, mapSize, map_size, "map_size", ((int[]){512, 512, 64}), 3, 1)
    READ_STR_FROM_JSON(parsed_map_json, author, author, "author", "Unknown", 0)
    (void) author;

    json_object_put(parsed_map_json);

    if (LoadMap(server, vxlMap, mapSize) == 0) {
        return;
    }

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
               server->protocol.colorTeam[0].colorArray,
               server->protocol.colorTeam[1].colorArray,
               server->protocol.currentGameMode,
               1);
}

static void SendJoiningData(Server* server, uint8 playerID)
{
    LOG_INFO("Sending state to %s (#%hhu)", server->player[playerID].name, playerID);
    for (uint8 i = 0; i < server->protocol.maxPlayers; ++i) {
        if (i != playerID && isPastJoinScreen(server, i)) {
            sendExistingPlayer(server, playerID, i);
        }
    }
    sendStateData(server, playerID);
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
            server->player[playerID].blockBuffer = NULL;
            sendMapStart(server, playerID);
            break;
        case STATE_LOADING_CHUNKS:
            sendMapChunks(server, playerID);
            break;
        case STATE_JOINING:
            SendJoiningData(server, playerID);
            break;
        case STATE_SPAWNING:
            server->player[playerID].HP             = 100;
            server->player[playerID].grenades       = 3;
            server->player[playerID].blocks         = 50;
            server->player[playerID].item           = 2;
            server->player[playerID].input          = 0;
            server->player[playerID].movForward     = 0;
            server->player[playerID].movBackwards   = 0;
            server->player[playerID].movLeft        = 0;
            server->player[playerID].movRight       = 0;
            server->player[playerID].jumping        = 0;
            server->player[playerID].crouching      = 0;
            server->player[playerID].sneaking       = 0;
            server->player[playerID].sprinting      = 0;
            server->player[playerID].primary_fire   = 0;
            server->player[playerID].secondary_fire = 0;
            server->player[playerID].alive          = 1;
            server->player[playerID].reloading      = 0;
            SetPlayerRespawnPoint(server, playerID);
            SendRespawn(server, playerID);
            LOG_INFO("Player %s (#%hhu) spawning at: %f %f %f",
                     server->player[playerID].name,
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

static void* WorldUpdate(void)
{
    for (uint8 playerID = 0; playerID < server.protocol.maxPlayers; ++playerID) {
        OnPlayerUpdate(&server, playerID);
        if (isPastJoinScreen(&server, playerID)) {
            uint64 time = getNanos();
            if (time - server.player[playerID].timers.timeSinceLastWU >=
                (uint64) (NANO_IN_SECOND / server.player[playerID].ups))
            {
                SendWorldUpdate(&server, playerID);
                server.player[playerID].timers.timeSinceLastWU = getNanos();
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
                    if (fp == NULL) {
                        perror("Unable to open/create Bans.json with error: ");
                        exit(EXIT_FAILURE);
                    }
                    fclose(fp);
                    root = json_object_new_object();
                    json_object_object_add(root, "Bans", json_object_new_array());
                    json_object_to_file("Bans.json", root);
                }
                IPStruct hostIP;
                hostIP.CIDR       = 24;
                hostIP.Union.ip32 = event.peer->address.host;
                struct json_object* array;
                json_object_object_get_ex(root, "Bans", &array);
                int count = json_object_array_length(array);
                for (int i = 0; i < count; ++i) {
                    struct json_object* objectAtIndex = json_object_array_get_idx(array, i);
                    const char*         IP;
                    const char*         startOfRangeString;
                    const char*         endOfRangeString;
                    READ_STR_FROM_JSON(
                    objectAtIndex, startOfRangeString, start_of_range, "start of range", "0.0.0.0", 1);
                    READ_STR_FROM_JSON(objectAtIndex, endOfRangeString, end_of_range, "end of range", "0.0.0.0", 1);
                    READ_STR_FROM_JSON(objectAtIndex, IP, IP, "IP", "0.0.0.0", 1);
                    IPStruct ipStruct;
                    IPStruct startOfRange, endOfRange;
                    if (formatStringToIP((char*) IP, &ipStruct) &&
                        (formatStringToIP((char*) startOfRangeString, &startOfRange) &&
                         formatStringToIP((char*) endOfRangeString, &endOfRange)))
                    {
                        if (IPInRange(hostIP, ipStruct, startOfRange, endOfRange)) {
                            const char* nameOfPlayer;
                            const char* reason;
                            double      time    = 0.0f;
                            uint64      timeNow = getNanos();
                            READ_STR_FROM_JSON(objectAtIndex, nameOfPlayer, Name, "Name", "Deuce", 0);
                            READ_STR_FROM_JSON(objectAtIndex, reason, Reason, "Reason", "None", 0);
                            READ_DOUBLE_FROM_JSON(objectAtIndex, time, Time, "Time", 0.0f, 0);
                            if (((long double) timeNow / NANO_IN_MINUTE) > time && time != 0) {
                                json_object_array_del_idx(array, i, 1);
                                json_object_to_file("Bans.json", root);
                                continue; // Continue searching for bans and delete all the old ones that match host IP
                                          // or its range
                            } else {
                                enet_peer_disconnect(event.peer, REASON_BANNED);

                                LOG_WARNING("Banned user %s tried to join with IP: %hhu.%hhu.%hhu.%hhu Banned for: %s",
                                            nameOfPlayer,
                                            hostIP.Union.ip[0],
                                            hostIP.Union.ip[1],
                                            hostIP.Union.ip[2],
                                            hostIP.Union.ip[3],
                                            reason);
                                bannedUser       = 1;
                                event.peer->data = (void*) ((size_t) server->protocol.maxPlayers - 1);
                                break;
                            }
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

                formatIPToString(server->player[playerID].name, server->player[playerID].ipStruct);
                snprintf(server->player[playerID].name, 6, "Limbo");
                char ipString[17];
                formatIPToString(ipString, server->player[playerID].ipStruct);
                LOG_INFO("Player %s (%s, #%hhu) connected", server->player[playerID].name, ipString, playerID);

                server->player[playerID].timers.sincePeriodicMessage = getNanos();
                server->player[playerID].currentPeriodicMessage      = server->periodicMessages;
                server->player[playerID].periodicDelayIndex          = 0;
                break;
            case ENET_EVENT_TYPE_DISCONNECT:
                playerID = (uint8) ((size_t) event.peer->data);
                if (playerID != server->protocol.maxPlayers - 1) {
                    sendIntelDrop(server, playerID);
                    sendPlayerLeft(server, playerID);
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

void ReadlineNewLine(int signal)
{
    (void) signal; // To prevent a warning about unused variable

    ctrlc = 1;
    printf("\n");
    LOG_INFO_WITHOUT_TIME("Are you sure you want to exit? (Y/n)");

    if (write(STDIN_FILENO, "\n", sizeof("\n")) != sizeof("\n")) {
        LOG_DEBUG("epic write fail");
    }
    rl_replace_line("", 0);
    rl_on_new_line();
    rl_redisplay();
}

void StopServer(void)
{
    server.running = 0;
}

static void* serverConsole(void* arg)
{
    (void) arg;
    char* buf;

    while ((buf = readline("\x1B[0;34mConsole> \x1B[0;37m")) != NULL) {
        if (ctrlc) { // "Are you sure?" prompt
            if (strcmp("n", buf) != 0) {
                break;
            }
            ctrlc = 0;
        } else if (strlen(buf) > 0) {
            add_history(buf);
            pthread_mutex_lock(&serverLock);
            handleCommands(&server, 255, buf, 1);
            pthread_mutex_unlock(&serverLock);
        }
        free(buf);
    }
    rl_clear_history();
    StopServer();

    sleep(5); // wait 5 seconds for the server to stop

    // if this thread is not dead at this point, then we need to stop the server by force >:)
    LOG_ERROR("Server did not respond for 5 seconds. Killing it with fire...");
    exit(-1);

    return 0;
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
    server.globalTimers.timeSinceStart = getNanos();
    LOG_STATUS("Welcome to SpadesX server");
    LOG_STATUS("Initializing ENet");

    if (enet_initialize() != 0) {
        LOG_ERROR("Failed to initalize ENet");
        exit(EXIT_FAILURE);
    }
    atexit(enet_deinitialize);

    if (pthread_mutex_init(&serverLock, NULL) != 0) {
        LOG_ERROR("Server mutex failed to initialize");
        exit(EXIT_FAILURE);
    }

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

    pthread_t masterThread;
    pthread_create(&masterThread, NULL, keepMasterAlive, (void*) &server);
    pthread_detach(masterThread);

    rl_catch_signals = 0;
    pthread_t console;
    pthread_create(&console, NULL, serverConsole, NULL);
    pthread_detach(console);

    signal(SIGINT, ReadlineNewLine);

    while (server.running) {
        pthread_mutex_lock(&serverLock);
        calculatePhysics();
        ServerUpdate(&server, 0);
        WorldUpdate();
        forPlayers();
        pthread_mutex_unlock(&serverLock);
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
    mapvxl_free(&server.map.map);

    pthread_mutex_destroy(&serverLock);

    printf("\n");
    LOG_STATUS("Exiting");
    enet_host_destroy(server.host);
}
