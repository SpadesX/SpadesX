#include "Server.h"

#include <enet/enet.h>
#include <json-c/json.h>
#include <json-c/json_object.h>
#include <stdio.h>
#include <string.h>

int raw_udp_intercept_callback(ENetHost* host, ENetEvent* event)
{
    (void)event;
    if (!strncmp((char*)host->receivedData, "HELLO", host->receivedDataLength)) {
        enet_socket_send(host->socket, &host->receivedAddress, &(ENetBuffer) { .data = "HI", .dataLength = 2 }, 1);
        return 1;
    } else if (!strncmp((char*)host->receivedData, "HELLOLAN", host->receivedDataLength)) {
        server_t* server = get_server();
        if (strlen(server->map_name) == 0) {
            return 1;
        }
        json_object* jobj                  = json_object_new_object();
        json_object* serverNameObject      = json_object_new_string(server->server_name);
        json_object* mapNameObject         = json_object_new_string(server->map_name);
        json_object* gamemodeObject        = json_object_new_string(server->gamemode_name);
        json_object* protocolVersionObject = json_object_new_string("0.75");
        json_object* currentPlayersObject  = json_object_new_int(server->protocol.num_users);
        json_object* maxPlayersObject      = json_object_new_int(server->protocol.max_players);

        json_object_object_add(jobj, "name", serverNameObject);
        json_object_object_add(jobj, "players_current", currentPlayersObject);
        json_object_object_add(jobj, "players_max", maxPlayersObject);
        json_object_object_add(jobj, "map", mapNameObject);
        json_object_object_add(jobj, "game_mode", gamemodeObject);
        json_object_object_add(jobj, "game_version", protocolVersionObject);

        const char* json = json_object_to_json_string(jobj);

        enet_socket_send(
            host->socket,
            &host->receivedAddress,
            &(ENetBuffer) { .data = (char*)json, .dataLength = strlen(json) },
            1);

        while (json_object_put(jobj) != 1) {
            // Free the memory from the JSON object
        }

        return 1;
    }
    return 0;
}
