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

#include <stdio.h>
// Include stdio before readline cause it needs it for some reason
#include <enet/enet.h>
#include <errno.h>
#include <json-c/json.h>
#include <json-c/json_object.h>
#include <json-c/json_types.h>
#include <json-c/json_util.h>
#include <math.h>
#include <pthread.h>
#include <readline/history.h>
#include <readline/readline.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

server_t        server;
pthread_mutex_t server_lock;

volatile int ctrlc = 0;

server_t* get_server(void)
{
    return &server; // Needed when we cant pass Server as argument into function
}

static void _string_nodes_free(string_node_t* root)
{
    string_node_t *el, *tmp;
    DL_FOREACH_SAFE(root, el, tmp)
    {
        free(el->string);
        DL_DELETE(root, el);
        free(el);
    }
}

static void _for_players(void)
{
    for (int player_id = 0; player_id < server.protocol.max_players; ++player_id) {
        if (is_past_join_screen(&server, player_id)) {
            uint64_t timeNow = get_nanos();
            if (server.player[player_id].primary_fire == 1 && server.player[player_id].reloading == 0) {
                if (server.player[player_id].weapon_clip > 0) {
                    uint64_t milliseconds = 0;
                    switch (server.player[player_id].weapon) {
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
                    if (diff_is_older_then(timeNow,
                                           &server.player[player_id].timers.since_last_weapon_input,
                                           NANO_IN_MILLI * milliseconds)) {
                        server.player[player_id].weapon_clip--;
                    }
                }
            } else if (server.player[player_id].primary_fire == 0 && server.player[player_id].reloading == 1) {
                switch (server.player[player_id].weapon) {
                    case WEAPON_RIFLE:
                    case WEAPON_SMG:
                    {
                        if (diff_is_older_then(timeNow,
                                               &server.player[player_id].timers.since_reload_start,
                                               NANO_IN_MILLI * (uint64_t) 2500)) {
                            uint8_t defaultAmmo = RIFLE_DEFAULT_CLIP;
                            if (server.player[player_id].weapon == WEAPON_SMG) {
                                defaultAmmo = SMG_DEFAULT_CLIP;
                            }
                            double newReserve = fmax(0,
                                                     server.player[player_id].weapon_reserve -
                                                     (defaultAmmo - server.player[player_id].weapon_clip));
                            server.player[player_id].weapon_clip +=
                            server.player[player_id].weapon_reserve - newReserve;
                            server.player[player_id].weapon_reserve = newReserve;
                            server.player[player_id].reloading      = 0;
                            send_weapon_reload(&server, player_id, 0, 0, 0);
                        }
                        break;
                    }
                    case WEAPON_SHOTGUN:
                    {
                        if (diff_is_older_then(timeNow,
                                               &server.player[player_id].timers.since_reload_start,
                                               NANO_IN_MILLI * (uint64_t) 500)) {
                            if (server.player[player_id].weapon_reserve == 0 ||
                                server.player[player_id].weapon_clip == SHOTGUN_DEFAULT_CLIP) {
                                server.player[player_id].reloading = 0;
                                break;
                            }
                            server.player[player_id].weapon_clip++;
                            server.player[player_id].weapon_reserve--;
                            if (server.player[player_id].weapon_clip == SHOTGUN_DEFAULT_CLIP) {
                                server.player[player_id].reloading = 0;
                            }
                            send_weapon_reload(&server, player_id, 0, 0, 0);
                        }
                        break;
                    }
                }
            }
            if (diff_is_older_then(
                timeNow,
                &server.player[player_id].timers.since_periodic_message,
                (uint64_t) (server.periodic_delays[server.player[player_id].periodic_delay_index] * 60) *
                NANO_IN_SECOND))
            {
                string_node_t* message;
                DL_FOREACH(server.periodic_messages, message)
                {
                    send_server_notice(&server, player_id, 0, message->string);
                }
                server.player[player_id].periodic_delay_index =
                fmin(server.player[player_id].periodic_delay_index + 1, 4);
            }
        } else if (server.player[player_id].state == STATE_PICK_SCREEN) {
            block_node_t *node, *tmp;
            LL_FOREACH_SAFE(server.player[player_id].blockBuffer, node, tmp)
            {
                if (node->type == 10) {
                    send_set_color_to_player(
                    &server, node->sender_id, player_id, node->color.r, node->color.g, node->color.b);
                    block_line_to_player(&server, node->sender_id, player_id, node->position, node->position_end);
                    send_set_color_to_player(&server,
                                             node->sender_id,
                                             player_id,
                                             server.player[node->sender_id].color.r,
                                             server.player[node->sender_id].color.g,
                                             server.player[node->sender_id].color.b);
                } else {
                    send_set_color_to_player(
                    &server, node->sender_id, player_id, node->color.r, node->color.g, node->color.b);
                    send_block_action_to_player(&server,
                                                node->sender_id,
                                                player_id,
                                                node->type,
                                                node->position.x,
                                                node->position.y,
                                                node->position.z);
                    send_set_color_to_player(&server,
                                             node->sender_id,
                                             player_id,
                                             server.player[node->sender_id].color.r,
                                             server.player[node->sender_id].color.g,
                                             server.player[node->sender_id].color.b);
                }
                LL_DELETE(server.player[player_id].blockBuffer, node);
                free(node);
            }
        } else if (is_past_state_data(&server, player_id)) {
        }
    }
}

static void* calculatePhysics(void)
{
    server.global_timers.update_time = get_nanos();
    if (server.global_timers.update_time - server.global_timers.last_update_time >= (1000000000 / 60)) {
        update_movement_and_grenades(&server);
        server.global_timers.last_update_time = get_nanos();
    }
    return 0;
}

static void ServerInit(server_t*   server,
                       uint32_t    connections,
                       const char* serverName,
                       const char* team1Name,
                       const char* team2Name,
                       uint8_t*    team1_color,
                       uint8_t*    team2_color,
                       uint8_t     gamemode,
                       uint8_t     reset)
{
    server->global_timers.update_time = server->global_timers.last_update_time = get_nanos();
    if (reset == 0) {
        server->protocol.num_players = 0;
        server->protocol.max_players = (connections <= 32) ? connections : 32;
    }

    server->s_map.compressed_map  = NULL;
    server->s_map.compressed_size = 0;
    server->protocol.input_flags  = 0;

    char    vxlMap[64];
    uint8_t index;
    if (reset == 0) {
        srand(time(NULL));
    }
    index                     = rand() % server->s_map.map_count;
    server->s_map.current_map = server->s_map.map_list;
    for (int i = 0; i <= index; ++i) {
        if (server->s_map.current_map->next != NULL) {
            server->s_map.current_map = server->s_map.current_map->next;
        } else {
            break; // Safety if we by some magical reason go beyond the list
        }
    }
    snprintf(
    server->map_name, fmin(strlen(server->s_map.current_map->string) + 1, 20), "%s", server->s_map.current_map->string);
    LOG_STATUS("Selecting %s as map", server->map_name);

    snprintf(vxlMap, 64, "%s.vxl", server->s_map.current_map->string);

    LOG_STATUS("Loading spawn ranges from map file");
    char mapConfig[64];
    snprintf(mapConfig, 64, "%s.json", server->s_map.current_map->string);

    struct json_object* parsed_map_json;
    parsed_map_json = json_object_from_file(mapConfig);

    int         team1Start[2];
    int         team2Start[2];
    int         team1End[2];
    int         team2End[2];
    int         map_size[3];
    const char* author;

    READ_INT_ARR_FROM_JSON(parsed_map_json, team1Start, team1_start, "team1 start", ((int[]){0, 0}), 2, 0)
    READ_INT_ARR_FROM_JSON(parsed_map_json, team1End, team1_end, "team1 end", ((int[]){10, 10}), 2, 0)
    READ_INT_ARR_FROM_JSON(parsed_map_json, team2Start, team2_start, "team2 start", ((int[]){20, 20}), 2, 0)
    READ_INT_ARR_FROM_JSON(parsed_map_json, team2End, team2_end, "team2 end", ((int[]){30, 30}), 2, 0)
    READ_INT_ARR_FROM_JSON(parsed_map_json, map_size, map_size, "map_size", ((int[]){512, 512, 64}), 3, 1)
    READ_STR_FROM_JSON(parsed_map_json, author, author, "author", "Unknown", 0)
    (void) author;

    json_object_put(parsed_map_json);

    if (map_load(server, vxlMap, map_size) == 0) {
        return;
    }

    vector3f_t empty   = {0, 0, 0};
    vector3f_t forward = {1, 0, 0};
    vector3f_t height  = {0, 0, 1};
    vector3f_t strafe  = {0, 1, 0};
    for (uint32_t i = 0; i < server->protocol.max_players; ++i) {
        init_player(server, i, reset, 0, empty, forward, strafe, height);
    }

    server->global_ab = 1;
    server->global_ak = 1;

    server->protocol.spawns[0].from.x = team1Start[0];
    server->protocol.spawns[0].from.y = team1Start[1];
    server->protocol.spawns[0].to.x   = team1End[0];
    server->protocol.spawns[0].to.y   = team1End[1];

    server->protocol.spawns[1].from.x = team2Start[0];
    server->protocol.spawns[1].from.y = team2Start[1];
    server->protocol.spawns[1].to.x   = team2End[0];
    server->protocol.spawns[1].to.y   = team2End[1];

    server->protocol.color_fog.r = 0x80;
    server->protocol.color_fog.g = 0xE8;
    server->protocol.color_fog.b = 0xFF;

    server->protocol.color_team[0].b = team1_color[B_CHANNEL];
    server->protocol.color_team[0].g = team1_color[G_CHANNEL];
    server->protocol.color_team[0].r = team1_color[R_CHANNEL];

    server->protocol.color_team[1].b = team2_color[B_CHANNEL];
    server->protocol.color_team[1].g = team2_color[G_CHANNEL];
    server->protocol.color_team[1].r = team2_color[R_CHANNEL];

    memcpy(server->protocol.name_team[0], team1Name, strlen(team1Name));
    memcpy(server->protocol.name_team[1], team2Name, strlen(team2Name));
    server->protocol.name_team[0][strlen(team1Name)] = '\0';
    server->protocol.name_team[1][strlen(team2Name)] = '\0';

    memcpy(server->server_name, serverName, strlen(serverName));
    server->server_name[strlen(serverName)] = '\0';
    gamemode_init(server, gamemode);
}

void server_reset(server_t* server)
{
    ServerInit(server,
               server->protocol.max_players,
               server->server_name,
               server->protocol.name_team[0],
               server->protocol.name_team[1],
               server->protocol.color_team[0].arr,
               server->protocol.color_team[1].arr,
               server->protocol.current_gamemode,
               1);
}

static void SendJoiningData(server_t* server, uint8_t player_id)
{
    LOG_INFO("Sending state to %s (#%hhu)", server->player[player_id].name, player_id);
    for (uint8_t i = 0; i < server->protocol.max_players; ++i) {
        if (i != player_id && is_past_join_screen(server, i)) {
            send_existing_player(server, player_id, i);
        }
    }
    send_state_data(server, player_id);
}

static uint8_t OnConnect(server_t* server)
{
    if (server->protocol.num_players == server->protocol.max_players) {
        return 0xFF;
    }
    uint8_t player_id;
    for (player_id = 0; player_id < server->protocol.max_players; ++player_id) {
        if (server->player[player_id].state == STATE_DISCONNECTED) {
            server->player[player_id].state = STATE_STARTING_MAP;
            break;
        }
    }
    server->protocol.num_players++;
    return player_id;
}

static void OnPlayerUpdate(server_t* server, uint8_t player_id)
{
    switch (server->player[player_id].state) {
        case STATE_STARTING_MAP:
            server->player[player_id].blockBuffer = NULL;
            send_map_start(server, player_id);
            break;
        case STATE_LOADING_CHUNKS:
            send_map_chunks(server, player_id);
            break;
        case STATE_JOINING:
            SendJoiningData(server, player_id);
            break;
        case STATE_SPAWNING:
            server->player[player_id].hp             = 100;
            server->player[player_id].grenades       = 3;
            server->player[player_id].blocks         = 50;
            server->player[player_id].item           = 2;
            server->player[player_id].input          = 0;
            server->player[player_id].move_forward   = 0;
            server->player[player_id].move_backwards = 0;
            server->player[player_id].move_left      = 0;
            server->player[player_id].move_right     = 0;
            server->player[player_id].jumping        = 0;
            server->player[player_id].crouching      = 0;
            server->player[player_id].sneaking       = 0;
            server->player[player_id].sprinting      = 0;
            server->player[player_id].primary_fire   = 0;
            server->player[player_id].secondary_fire = 0;
            server->player[player_id].alive          = 1;
            server->player[player_id].reloading      = 0;
            set_player_respawn_point(server, player_id);
            send_respawn(server, player_id);
            LOG_INFO("Player %s (#%hhu) spawning at: %f %f %f",
                     server->player[player_id].name,
                     player_id,
                     server->player[player_id].movement.position.x,
                     server->player[player_id].movement.position.y,
                     server->player[player_id].movement.position.z);
            break;
        case STATE_WAITING_FOR_RESPAWN:
        {
            if (time(NULL) - server->player[player_id].timers.start_of_respawn_wait >=
                server->player[player_id].respawn_time) {
                server->player[player_id].state = STATE_SPAWNING;
            }
            break;
        }
        case STATE_READY:
            // send data
            if (server->master.enable_master_connection == 1) {
                if (server->player[player_id].told_to_master == 0) {
                    master_update(server);
                    server->player[player_id].told_to_master = 1;
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
    for (uint8_t player_id = 0; player_id < server.protocol.max_players; ++player_id) {
        OnPlayerUpdate(&server, player_id);
        if (is_past_join_screen(&server, player_id)) {
            uint64_t time = get_nanos();
            if (time - server.player[player_id].timers.time_since_last_wu >=
                (uint64_t) (NANO_IN_SECOND / server.player[player_id].ups))
            {
                send_world_update(&server, player_id);
                server.player[player_id].timers.time_since_last_wu = get_nanos();
            }
        }
    }
    return 0;
}

static void ServerUpdate(server_t* server, int timeout)
{

    ENetEvent event;
    while (enet_host_service(server->host, &event, timeout) > 0) {
        uint8_t bannedUser = 0;
        uint8_t player_id;
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
                ip_t hostIP;
                hostIP.cidr = 24;
                hostIP.ip32 = event.peer->address.host;
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
                    ip_t ipStruct;
                    ip_t startOfRange, endOfRange;
                    if (format_str_to_ip((char*) IP, &ipStruct) &&
                        (format_str_to_ip((char*) startOfRangeString, &startOfRange) &&
                         format_str_to_ip((char*) endOfRangeString, &endOfRange)))
                    {
                        if (ip_in_range(hostIP, ipStruct, startOfRange, endOfRange)) {
                            const char* nameOfPlayer;
                            const char* reason;
                            double      time    = 0.0f;
                            uint64_t    timeNow = get_nanos();
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
                                            hostIP.ip[0],
                                            hostIP.ip[1],
                                            hostIP.ip[2],
                                            hostIP.ip[3],
                                            reason);
                                bannedUser       = 1;
                                event.peer->data = (void*) ((size_t) server->protocol.max_players - 1);
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
                player_id = OnConnect(server);
                if (player_id == 0xFF) {
                    enet_peer_disconnect_now(event.peer, REASON_SERVER_FULL);
                    LOG_WARNING("Server full. Kicking player");
                    break;
                }
                server->player[player_id].peer    = event.peer;
                event.peer->data                  = (void*) ((size_t) player_id);
                server->player[player_id].hp      = 100;
                server->player[player_id].ip.ip32 = event.peer->address.host;

                format_ip_to_str(server->player[player_id].name, server->player[player_id].ip);
                snprintf(server->player[player_id].name, 6, "Limbo");
                char ipString[17];
                format_ip_to_str(ipString, server->player[player_id].ip);
                LOG_INFO("Player %s (%s, #%hhu) connected", server->player[player_id].name, ipString, player_id);

                server->player[player_id].timers.since_periodic_message = get_nanos();
                server->player[player_id].current_periodic_message      = server->periodic_messages;
                server->player[player_id].periodic_delay_index          = 0;
                break;
            case ENET_EVENT_TYPE_DISCONNECT:
                player_id = (uint8_t) ((size_t) event.peer->data);
                if (player_id != server->protocol.max_players - 1) {
                    send_intel_drop(server, player_id);
                    send_player_left(server, player_id);
                    vector3f_t empty   = {0, 0, 0};
                    vector3f_t forward = {1, 0, 0};
                    vector3f_t height  = {0, 0, 1};
                    vector3f_t strafe  = {0, 1, 0};
                    init_player(server, player_id, 0, 1, empty, forward, strafe, height);
                    server->protocol.num_players--;
                    server->protocol.num_team_users[server->player[player_id].team]--;
                    if (server->master.enable_master_connection == 1) {
                        master_update(server);
                    }
                }
                break;
            case ENET_EVENT_TYPE_RECEIVE:
            {
                stream_t stream = {event.packet->data, event.packet->dataLength, 0};
                player_id       = (uint8_t) ((size_t) event.peer->data);
                on_packet_received(server, player_id, &stream);
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
            pthread_mutex_lock(&server_lock);
            command_handle(&server, 255, buf, 1);
            pthread_mutex_unlock(&server_lock);
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

void server_start(uint16_t       port,
                  uint32_t       connections,
                  uint32_t       channels,
                  uint32_t       inBandwidth,
                  uint32_t       outBandwidth,
                  uint8_t        master,
                  string_node_t* map_list,
                  uint8_t        map_count,
                  string_node_t* welcomeMessageList,
                  uint8_t        welcomeMessageListLen,
                  string_node_t* periodicMessageList,
                  uint8_t        periodicMessageListLen,
                  uint8_t*       periodicDelays,
                  const char*    managerPasswd,
                  const char*    adminPasswd,
                  const char*    modPasswd,
                  const char*    guardPasswd,
                  const char*    trustedPasswd,
                  const char*    serverName,
                  const char*    team1Name,
                  const char*    team2Name,
                  uint8_t*       team1_color,
                  uint8_t*       team2_color,
                  uint8_t        gamemode)
{
    server.global_timers.time_since_start = get_nanos();
    LOG_STATUS("Welcome to SpadesX server");
    LOG_STATUS("Initializing ENet");

    if (enet_initialize() != 0) {
        LOG_ERROR("Failed to initalize ENet");
        exit(EXIT_FAILURE);
    }
    atexit(enet_deinitialize);

    if (pthread_mutex_init(&server_lock, NULL) != 0) {
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

    server.host->intercept = &raw_udp_intercept_callback;

    LOG_STATUS("Intializing server");
    server.running                = 1;
    server.s_map.map_list         = map_list;
    server.s_map.map_count        = map_count;
    server.welcome_messages       = welcomeMessageList;
    server.welcome_messages_count = welcomeMessageListLen;
    server.periodic_messages      = periodicMessageList;
    server.periodic_message_count = periodicMessageListLen;
    server.periodic_delays        = periodicDelays;
    ServerInit(&server, connections, serverName, team1Name, team2Name, team1_color, team2_color, gamemode, 0);

    command_populate_all(&server);
    init_packets(&server);

    server.master.enable_master_connection = master;
    server.manager_passwd                  = managerPasswd;
    server.admin_passwd                    = adminPasswd;
    server.mod_passwd                      = modPasswd;
    server.guard_passwd                    = guardPasswd;
    server.trusted_passwd                  = trustedPasswd;

    if (server.running) {
        LOG_STATUS("Server started");
    }
    if (server.master.enable_master_connection == 1) {
        master_connect(&server, port);
    }
    server.master.time_since_last_send = time(NULL);

    pthread_t masterThread;
    pthread_create(&masterThread, NULL, master_keep_alive, (void*) &server);
    pthread_detach(masterThread);

    rl_catch_signals = 0;
    pthread_t console;
    pthread_create(&console, NULL, serverConsole, NULL);
    pthread_detach(console);

    signal(SIGINT, ReadlineNewLine);

    while (server.running) {
        pthread_mutex_lock(&server_lock);
        calculatePhysics();
        ServerUpdate(&server, 0);
        WorldUpdate();
        _for_players();
        pthread_mutex_unlock(&server_lock);
        sleep(0);
    }
    while (server.s_map.compressed_map) {
        server.s_map.compressed_map = queue_pop(server.s_map.compressed_map);
    }
    free(server.s_map.compressed_map);

    command_free_all(&server);
    free_all_packets(&server);

    _string_nodes_free(server.welcome_messages);
    _string_nodes_free(server.s_map.map_list);
    _string_nodes_free(server.periodic_messages);
    mapvxl_free(&server.s_map.map);

    pthread_mutex_destroy(&server_lock);

    printf("\n");
    LOG_STATUS("Exiting");
    enet_host_destroy(server.host);
}
