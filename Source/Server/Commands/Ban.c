#include <Server/Commands/CommandManager.h>
#include <Server/ParseConvert.h>
#include <Server/Server.h>
#include <Util/Log.h>
#include <Util/Nanos.h>
#include <Util/Notice.h>
#include <json-c/json_object.h>
#include <json-c/json_util.h>

void cmd_ban_custom(void* p_server, command_args_t arguments)
{
    server_t* server = (server_t*) p_server;
    ip_t      ip;
    char*     reason;
    uint8_t   customBan = 0;
    uint8_t   player_id = 33;
    if (arguments.argc == 2) {
        float time = 0.0f;
        if (strcmp(arguments.argv[0], "/pban") == 0) {
            time = 0.0f;
        } else if (strcmp(arguments.argv[0], "/hban") == 0) {
            time = 360.0f;
        } else if (strcmp(arguments.argv[0], "/dban") == 0) {
            time = 1440.0f;
        } else if (strcmp(arguments.argv[0], "/wban") == 0) {
            time = 10080.0f;
        } else if (strcmp(arguments.argv[0], "/mban") == 0) {
            time = 44640.0f;
        } else if (strcmp(arguments.argv[0], "/ban") == 0) {
            customBan = 1;
        } else {
            LOG_WARNING("Cant recognize ban command"); // Should never happen
        }

        if (parse_ip(arguments.argv[1], &ip, &reason)) {
            if (customBan) {
                char* temp = reason;
                parse_float(++temp, &time, &reason);
            }
            if (time > 0) {
                cmd_generate_ban(
                server, arguments, ((long double) get_nanos() / (uint64_t) NANO_IN_MINUTE) + time, ip, reason);
            } else {
                cmd_generate_ban(server, arguments, time, ip, reason);
            }
        } else if (parse_player(arguments.argv[1], &player_id, &reason)) {
            player_t* player;
            HASH_FIND(hh, server->players, &player_id, sizeof(player_id), player);
            if (player == NULL || player->state == STATE_DISCONNECTED) {
                send_server_notice(arguments.player, arguments.console, "Player with ID %hhu does not exist");
                return;
            }
            if (customBan) {
                char* temp = reason;
                parse_float(++temp, &time, &reason);
            }
            ip.ip32 = player->ip.ip32;
            ip.cidr = player->ip.cidr;
            if (time > 0) {
                cmd_generate_ban(
                server, arguments, ((long double) get_nanos() / (uint64_t) NANO_IN_MINUTE) + time, ip, reason);
            } else {
                cmd_generate_ban(server, arguments, time, ip, reason);
            }
        } else {
            send_server_notice(arguments.player, arguments.console, "Invalid IP format");
        }
    } else {
        send_server_notice(arguments.player, arguments.console, "You did not enter IP or entered incorrect argument");
    }
}

void cmd_ban_range(void* p_server, command_args_t arguments)
{
    server_t* server = (server_t*) p_server;
    ip_t      startRange;
    ip_t      endRange;
    char*     reason;
    char*     end;
    if (arguments.argc == 2) {
        if (parse_ip(arguments.argv[1], &startRange, &end) && parse_ip(++end, &endRange, &reason)) {
            char* temp = reason;
            float time = 0.0f;
            parse_float(++temp, &time, &reason);
            char ipStringStart[16];
            char ipStringEnd[16];
            format_ip_to_str(ipStringStart, startRange); // Reformatting the IP to avoid stuff like 001.02.3.4
            format_ip_to_str(ipStringEnd, endRange);
            struct json_object* array;
            struct json_object* ban        = json_object_new_object();
            struct json_object* root       = json_object_from_file("Bans.json");
            char*               nameString = "Deuce";
            json_object_object_get_ex(root, "Bans", &array);

            uint8_t   banned = 0;
            player_t *connected_player, *tmp;
            HASH_ITER(hh, server->players, connected_player, tmp)
            {
                if (connected_player->state != STATE_DISCONNECTED &&
                    octets_in_range(startRange, endRange, connected_player->ip))
                {
                    if (banned == 0) {
                        nameString = connected_player->name;
                        banned     = 1; // Do not add multiples of the same IP. Its pointless.
                    }
                    enet_peer_disconnect(connected_player->peer, REASON_BANNED);
                }
            }
            json_object_object_add(ban, "Name", json_object_new_string(nameString));
            json_object_object_add(ban, "start_of_range", json_object_new_string(ipStringStart));
            json_object_object_add(ban, "end_of_range", json_object_new_string(ipStringEnd));
            if (time == 0) {
                json_object_object_add(
                ban, "Time", json_object_new_double(((long double) get_nanos() / (uint64_t) NANO_IN_MINUTE) + time));
            } else {
                json_object_object_add(ban, "Time", json_object_new_double(time));
            }

            if (*reason == 32 && strlen(++reason) > 0) {
                json_object_object_add(ban, "Reason", json_object_new_string(reason));
            }
            json_object_array_add(array, ban);
            json_object_to_file("Bans.json", root);
            json_object_put(root);
            send_server_notice(arguments.player,
                               arguments.console,
                               "IP range %s-%s has been permanently banned",
                               ipStringStart,
                               ipStringEnd);
        } else {
            send_server_notice(arguments.player, arguments.console, "Invalid IP format");
        }
    } else {
        send_server_notice(arguments.player, arguments.console, "You did not enter IP or entered incorrect argument");
    }
}
