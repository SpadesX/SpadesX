// Copyright DarkNeutrino 2021
#include "Server.h"
#include "Structs.h"

#include <Types.h>
#include <JSONHelpers.h>
#include <json-c/json.h>
#include <json-c/json_object.h>
#include <stdio.h>
#include <string.h>

int main(void)
{
    uint16 port;
    uint8  master;
    uint8  gamemode;
    const char* managerPasswd;
    const char* adminPasswd;
    const char* modPasswd;
    const char* guardPasswd;
    const char* trustedPasswd;
    const char* serverName;
    const char* team1Name;
    const char* team2Name;
    uint8       team1Color[3];
    uint8       team2Color[3];

    struct json_object* parsed_json;
    struct json_object* mapInConfig;

    parsed_json = json_object_from_file("config.json");
    READ_INT_FROM_JSON(parsed_json, port, port, "port number", DEFAULT_SERVER_PORT)
    READ_INT_FROM_JSON(parsed_json, master, master, "master variable", 1)
    READ_INT_FROM_JSON(parsed_json, gamemode, gamemode, "gamemode", 0)
    if (json_object_object_get_ex(parsed_json, "map", &mapInConfig) == 0) {
        printf("Failed to find map name in config\n");
        return -1;
    }
    READ_STR_FROM_JSON(parsed_json, managerPasswd, manager_password, "manager password", "")
    READ_STR_FROM_JSON(parsed_json, adminPasswd, admin_password, "admin password", "")
    READ_STR_FROM_JSON(parsed_json, modPasswd, moderator_password, "moderator password", "")
    READ_STR_FROM_JSON(parsed_json, guardPasswd, guard_password, "guard password", "")
    READ_STR_FROM_JSON(parsed_json, trustedPasswd, trusted_password, "trusted password", "")
    READ_STR_FROM_JSON(parsed_json, serverName, server_name, "server name", "SpadesX Server")
    READ_STR_FROM_JSON(parsed_json, team1Name, team1_name, "team1 name", "Blue")
    READ_STR_FROM_JSON(parsed_json, team2Name, team2_name, "team2 name", "Red")
    // array requires additional pair of parentheses because of how macros work in C
    READ_INT_ARR_FROM_JSON(parsed_json, team1Color, team1_color, "team1 color", ((uint8[]){0, 0, 255}), 3)
    READ_INT_ARR_FROM_JSON(parsed_json, team2Color, team2_color, "team2 color", ((uint8[]){255, 0, 0}), 3)

    uint8 mapArrayLen = json_object_array_length(mapInConfig);
    char  mapArray[mapArrayLen][64];

    for (int i = 0; i < mapArrayLen; ++i) {
        json_object* temp      = json_object_array_get_idx(mapInConfig, i);
        uint8        stringLen = json_object_get_string_len(temp);
        if (stringLen > 64) {
            continue;
        }
        memcpy(mapArray[i], json_object_get_string(temp), stringLen + 1);
    }

    StartServer(port,
                64,
                2,
                0,
                0,
                master,
                mapArray,
                mapArrayLen,
                managerPasswd,
                adminPasswd,
                modPasswd,
                guardPasswd,
                trustedPasswd,
                serverName,
                team1Name,
                team2Name,
                team1Color,
                team2Color,
                gamemode);

    json_object_put(parsed_json);

    return 0;
}
