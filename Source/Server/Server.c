// Copyright DarkNeutrino 2021
#include <Server/Commands/Commands.h>
#include <Server/Console.h>
#include <Server/Gamemodes/Gamemodes.h>
#include <Server/Map.h>
#include <Server/Master.h>
#include <Server/Packets/Packets.h>
#include <Server/ParseConvert.h>
#include <Server/Ping.h>
#include <Server/Player.h>
#include <Server/Server.h>
#include <Server/Structs/ServerStruct.h>
#include <Server/ServerStartOptions.h>
#include <Util/Checks/PlayerChecks.h>
#include <Util/Compress.h>
#include <Util/DataStream.h>
#include <Util/Enums.h>
#include <Util/JSONHelpers.h>
#include <Util/Line.h>
#include <Util/Log.h>
#include <Util/Nanos.h>
#include <Util/Physics.h>
#include <Util/Queue.h>
#include <Util/Types.h>
#include <Util/Utlist.h>
#include <libmapvxl/libmapvxl.h>
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

static void* _calculate_physics(void)
{
    server.global_timers.update_time = get_nanos();
    if (server.global_timers.update_time - server.global_timers.last_update_time >= (1000000000 / 60)) {
        update_movement_and_grenades(&server);
        server.global_timers.last_update_time = get_nanos();
    }
    return 0;
}

static void _server_init(server_t*   server,
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

    char    vxl_map[64];
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

    snprintf(vxl_map, 64, "%s.vxl", server->s_map.current_map->string);

    LOG_STATUS("Loading spawn ranges from map file");
    char mapConfig[64];
    snprintf(mapConfig, 64, "%s.json", server->s_map.current_map->string);

    struct json_object* parsed_map_json;
    parsed_map_json = json_object_from_file(mapConfig);

    int         team1_start[2];
    int         team2_start[2];
    int         team1_end[2];
    int         team2_end[2];
    int         map_size[3];
    const char* author;

    READ_INT_ARR_FROM_JSON(parsed_map_json, team1_start, team1_start, "team1 start", ((int[]){0, 0}), 2, 0)
    READ_INT_ARR_FROM_JSON(parsed_map_json, team1_end, team1_end, "team1 end", ((int[]){10, 10}), 2, 0)
    READ_INT_ARR_FROM_JSON(parsed_map_json, team2_start, team2_start, "team2 start", ((int[]){20, 20}), 2, 0)
    READ_INT_ARR_FROM_JSON(parsed_map_json, team2_end, team2_end, "team2 end", ((int[]){30, 30}), 2, 0)
    READ_INT_ARR_FROM_JSON(parsed_map_json, map_size, map_size, "map_size", ((int[]){512, 512, 64}), 3, 1)
    READ_STR_FROM_JSON(parsed_map_json, author, author, "author", "Unknown", 0)
    (void) author;

    json_object_put(parsed_map_json);

    if (map_load(server, vxl_map, map_size) == 0) {
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

    server->protocol.spawns[0].from.x = team1_start[0];
    server->protocol.spawns[0].from.y = team1_start[1];
    server->protocol.spawns[0].to.x   = team1_end[0];
    server->protocol.spawns[0].to.y   = team1_end[1];

    server->protocol.spawns[1].from.x = team2_start[0];
    server->protocol.spawns[1].from.y = team2_start[1];
    server->protocol.spawns[1].to.x   = team2_end[0];
    server->protocol.spawns[1].to.y   = team2_end[1];

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
    _server_init(server,
                 server->protocol.max_players,
                 server->server_name,
                 server->protocol.name_team[0],
                 server->protocol.name_team[1],
                 server->protocol.color_team[0].arr,
                 server->protocol.color_team[1].arr,
                 server->protocol.current_gamemode,
                 1);
}

static void* _world_update(void)
{
    for (uint8_t player_id = 0; player_id < server.protocol.max_players; ++player_id) {
        on_player_update(&server, player_id);
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

static void _server_update(server_t* server, int timeout)
{

    ENetEvent event;
    while (enet_host_service(server->host, &event, timeout) > 0) {
        uint8_t player_id;
        switch (event.type) {
            case ENET_EVENT_TYPE_NONE:
                LOG_STATUS("Event of type none received. Ignoring");
                break;
            case ENET_EVENT_TYPE_CONNECT:
                on_new_player_connection(server, &event);
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

void stop_server(void)
{
    server.running = 0;
}


void server_start(struct server_start_options options)
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
    address.port = options.port;

    LOG_STATUS("Creating server at port %d", options.port);

    server.host = enet_host_create(&address, options.connections, options.channels, options.in_bandwidth, options.out_bandwidth);
    if (server.host == NULL) {
        LOG_ERROR("Failed to create server");
        exit(EXIT_FAILURE);
    }

    if (enet_host_compress_with_range_coder(server.host) != 0) {
        LOG_WARNING("Compress with range coder failed");
    }

    server.port = options.port;

    server.host->intercept = &raw_udp_intercept_callback;

    LOG_STATUS("Intializing server");
    server.running                = 1;
    server.s_map.map_list         = options.map_list;
    server.s_map.map_count        = options.map_count;
    server.welcome_messages       = options.welcome_message_list;
    server.welcome_messages_count = options.welcome_message_list_len;
    server.periodic_messages      = options.periodic_message_list;
    server.periodic_message_count = options.periodic_message_list_len;
    server.periodic_delays        = options.periodic_delays;
    _server_init(&server, options.connections, options.server_name, options.team1_name, options.team2_name, options.team1_color, options.team2_color, options.gamemode, 0);

    command_populate_all(&server);
    init_packets(&server);

    server.master.enable_master_connection = options.master;
    server.manager_passwd                  = options.manager_password;
    server.admin_passwd                    = options.admin_password;
    server.mod_passwd                      = options.mod_password;
    server.guard_passwd                    = options.guard_password;
    server.trusted_passwd                  = options.trusted_password;

    if (server.running) {
        LOG_STATUS("Server started");
    }
    if (server.master.enable_master_connection == 1) {
        master_connect(&server, options.port);
    }
    server.master.time_since_last_send = time(NULL);

    pthread_t masterThread;
    pthread_create(&masterThread, NULL, master_keep_alive, (void*) &server);
    pthread_detach(masterThread);

    rl_catch_signals = 0;
    pthread_t console;
    pthread_create(&console, NULL, server_console, (void*) &server);
    pthread_detach(console);

    signal(SIGINT, readline_new_line);

    while (server.running) {
        pthread_mutex_lock(&server_lock);
        _calculate_physics();
        _server_update(&server, 0);
        _world_update();
        for_players(&server);
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
