// Copyright DarkNeutrino 2021
#include "Server.h"
#include "Structs.h"

#include <Types.h>
#include <json-c/json.h>
#include <stdio.h>

int main(void)
{
    struct json_object* parsed_json;
    struct json_object* portInConfig;
    struct json_object* masterInConfig;
    struct json_object* mapInConfig;
    struct json_object* managerPasswdInConfig;
    struct json_object* adminPasswdInConfig;
    struct json_object* modPasswdInConfig;
    struct json_object* guardPasswdInConfig;
    struct json_object* trustedPasswdInConfig;

    uint16 port   = DEFAULT_SERVER_PORT;
    uint8  master = 1;

    parsed_json = json_object_from_file("config.json");
    if (json_object_object_get_ex(parsed_json, "port", &portInConfig) == 0) {
        printf("Failed to find port number in config\n");
        return -1;
    }
    if (json_object_object_get_ex(parsed_json, "master", &masterInConfig) == 0) {
        printf("Failed to find master variable in config\n");
        return -1;
    }
    if (json_object_object_get_ex(parsed_json, "map", &mapInConfig) == 0) {
        printf("Failed to find map name in config\n");
        return -1;
    }
    if (json_object_object_get_ex(parsed_json, "manager_password", &managerPasswdInConfig) == 0) {
        printf("Failed to find manager password in config\n");
        return -1;
    }
    if (json_object_object_get_ex(parsed_json, "admin_password", &adminPasswdInConfig) == 0) {
        printf("Failed to find admin password in config\n");
        return -1;
    }
    if (json_object_object_get_ex(parsed_json, "moderator_password", &modPasswdInConfig) == 0) {
        printf("Failed to find moderator password in config\n");
        return -1;
    }
    if (json_object_object_get_ex(parsed_json, "guard_password", &guardPasswdInConfig) == 0) {
        printf("Failed to find guard password in config\n");
        return -1;
    }
    if (json_object_object_get_ex(parsed_json, "trusted_password", &trustedPasswdInConfig) == 0) {
        printf("Failed to find trusted password in config\n");
        return -1;
    }

    const char* map           = json_object_get_string(mapInConfig);
    port                      = json_object_get_int(portInConfig);
    master                    = json_object_get_int(masterInConfig);
    const char* managerPasswd = json_object_get_string(managerPasswdInConfig);
    const char* adminPasswd   = json_object_get_string(adminPasswdInConfig);
    const char* modPasswd     = json_object_get_string(modPasswdInConfig);
    const char* guardPasswd   = json_object_get_string(guardPasswdInConfig);
    const char* trustedPasswd = json_object_get_string(trustedPasswdInConfig);

    StartServer(port, 64, 2, 0, 0, master, map, managerPasswd, adminPasswd, modPasswd, guardPasswd, trustedPasswd);
    return 0;
}
