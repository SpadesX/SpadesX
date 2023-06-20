// Copyright DarkNeutrino 2021
#include <Server/Server.h>
#include <Server/Structs/ServerStruct.h>
#include <Server/Structs/StartStruct.h>
#include <Util/JSONHelpers.h>
#include <Util/Log.h>
#include <Util/Types.h>
#include <Util/Utlist.h>
#include <Util/Alloc.h>
#include <json-c/json.h>
#include <json-c/json_object.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void)
{
    uint16_t    port;
    uint8_t     master;
    uint8_t     gamemode;
    uint8_t     capture_limit;
    const char* manager_passwd;
    const char* admin_passwd;
    const char* mod_passwd;
    const char* guard_passwd;
    const char* trusted_passwd;
    const char* server_name;
    const char* team1_name;
    const char* team2_name;
    uint8_t     team1_color[3];
    uint8_t     team2_color[3];
    uint8_t     periodic_delays[5];

    struct json_object* parsed_json;
    struct json_object* map_in_config;
    struct json_object* welcome_message_in_config;
    struct json_object* periodic_message_in_config;

    parsed_json = json_object_from_file("config.json");
    READ_INT_FROM_JSON(parsed_json, port, port, "port number", DEFAULT_SERVER_PORT, 0)
    READ_INT_FROM_JSON(parsed_json, master, master, "master variable", 1, 0)
    READ_INT_FROM_JSON(parsed_json, gamemode, gamemode, "gamemode", 0, 0)
    READ_INT_FROM_JSON(parsed_json, capture_limit, capture_limit, "capture limit", 10, 0)
    if (json_object_object_get_ex(parsed_json, "map", &map_in_config) == 0) {
        LOG_ERROR("Failed to find map name in config");
        return -1;
    }
    if (json_object_object_get_ex(parsed_json, "welcome_messages", &welcome_message_in_config) == 0) {
        LOG_ERROR("Failed to find welcome messages in config");
        return -1;
    }
    if (json_object_object_get_ex(parsed_json, "periodic_messages", &periodic_message_in_config) == 0) {
        LOG_ERROR("Failed to find periodic messages in config");
        return -1;
    }
    READ_STR_FROM_JSON(parsed_json, manager_passwd, manager_password, "manager password", "", 0);
    READ_STR_FROM_JSON(parsed_json, admin_passwd, admin_password, "admin password", "", 0);
    READ_STR_FROM_JSON(parsed_json, mod_passwd, moderator_password, "moderator password", "", 0);
    READ_STR_FROM_JSON(parsed_json, guard_passwd, guard_password, "guard password", "", 0);
    READ_STR_FROM_JSON(parsed_json, trusted_passwd, trusted_password, "trusted password", "", 0);
    READ_STR_FROM_JSON(parsed_json, server_name, server_name, "server name", "SpadesX Server", 0);
    READ_STR_FROM_JSON(parsed_json, team1_name, team1_name, "team1 name", "Blue", 0);
    READ_STR_FROM_JSON(parsed_json, team2_name, team2_name, "team2 name", "Red", 0);
    // array requires additional pair of parentheses because of how macros work in C
    READ_INT_ARR_FROM_JSON(parsed_json, team1_color, team1_color, "team1 color", ((uint8_t[]){255, 0, 0}), 3, 0);
    READ_INT_ARR_FROM_JSON(parsed_json, team2_color, team2_color, "team2 color", ((uint8_t[]){0, 0, 255}), 3, 0);
    READ_INT_ARR_FROM_JSON(
    parsed_json, periodic_delays, periodic_delays, "periodic delays", ((uint8_t[]){1, 5, 10, 30, 60}), 5, 1);

    uint8_t        map_list_len = json_object_array_length(map_in_config);
    string_node_t* map_list     = NULL;

    for (int i = 0; i < map_list_len; ++i) {
        json_object*   temp       = json_object_array_get_idx(map_in_config, i);
        uint8_t        string_len = json_object_get_string_len(temp);
        string_node_t* map_name   = spadesx_malloc(sizeof(string_node_t));
        char*          map        = spadesx_malloc(sizeof(char) * string_len + 1);
        memcpy(map, json_object_get_string(temp), string_len + 1);
        map_name->string = map;
        DL_APPEND(map_list, map_name);
    }

    uint8_t        welcome_message_list_len = json_object_array_length(welcome_message_in_config);
    string_node_t* welcome_message_list     = NULL;

    for (int i = 0; i < welcome_message_list_len; ++i) {
        json_object* temp      = json_object_array_get_idx(welcome_message_in_config, i);
        uint8_t      stringLen = json_object_get_string_len(temp);
        if (stringLen > 128) {
            LOG_WARNING(
            "Welcome message in config with index %d exceeds 128 characters. Please use shorter message. IGNORING", i);
            continue;
        }
        string_node_t* welcome_message        = spadesx_malloc(sizeof(string_node_t));
        char*          welcome_message_string = spadesx_malloc(sizeof(char) * stringLen + 1);
        memcpy(welcome_message_string, json_object_get_string(temp), stringLen + 1);
        welcome_message->string = welcome_message_string;
        DL_APPEND(welcome_message_list, welcome_message);
    }

    uint8_t        periodic_message_list_len = json_object_array_length(periodic_message_in_config);
    string_node_t* periodic_message_list     = NULL;

    for (int i = 0; i < periodic_message_list_len; ++i) {
        json_object* temp       = json_object_array_get_idx(periodic_message_in_config, i);
        uint8_t      string_len = json_object_get_string_len(temp);
        if (string_len > 128) {
            LOG_WARNING(
            "Periodic message in config with index %d exceeds 128 characters. Please use shorter message. IGNORING", i);
            continue;
        }
        string_node_t* periodic_message        = spadesx_malloc(sizeof(string_node_t));
        char*          periodic_message_string = spadesx_malloc(sizeof(char) * string_len + 1);
        memcpy(periodic_message_string, json_object_get_string(temp), string_len + 1);
        periodic_message->string = periodic_message_string;
        DL_APPEND(periodic_message_list, periodic_message);
    }

    server_args args = {.port                      = port,
                        .connections               = 64,
                        .channels                  = 2,
                        .in_bandwidth              = 0,
                        .out_bandwidth             = 0,
                        .master                    = master,
                        .map_list                  = map_list,
                        .map_count                 = map_list_len,
                        .welcome_message_list      = welcome_message_list,
                        .welcome_message_list_len  = welcome_message_list_len,
                        .periodic_message_list     = periodic_message_list,
                        .periodic_message_list_len = periodic_message_list_len,
                        .periodic_delays           = periodic_delays,
                        .manager_password          = manager_passwd,
                        .admin_password            = admin_passwd,
                        .mod_password              = mod_passwd,
                        .guard_password            = guard_passwd,
                        .trusted_password          = trusted_passwd,
                        .server_name               = server_name,
                        .team1_name                = team1_name,
                        .team2_name                = team2_name,
                        .team1_color               = team1_color,
                        .team2_color               = team2_color,
                        .gamemode                  = gamemode,
                        .capture_limit             = capture_limit};

    server_start(args);

    while (json_object_put(parsed_json) != 1) {
        // Free the memory from the JSON object
    }

    return 0;
}
