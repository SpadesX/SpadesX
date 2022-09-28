#include "Commands.h"

#include "Master.h"
#include "Packets.h"
#include "ParseConvert.h"
#include "Protocol.h"
#include "Server.h"
#include "Structs.h"
#include "Util/Enums.h"
#include "Util/JSONHelpers.h"
#include "Util/Log.h"
#include "Util/Types.h"
#include "Util/Uthash.h"
#include "Util/Utlist.h"

#include <ctype.h>
#include <enet/enet.h>
#include <json-c/json.h>
#include <json-c/json_object.h>
#include <json-c/json_util.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

static uint32_t _cmds_compare(command_t* first, command_t* second)
{
    return strcmp(first->id, second->id);
}

static void _cmd_generate_ban(server_t* server, command_args_t arguments, float time, ip_t ip, char* reason)
{
    char ipString[19];
    format_ip_to_str(ipString, ip);
    struct json_object* array;
    struct json_object* ban        = json_object_new_object();
    struct json_object* root       = json_object_from_file("Bans.json");
    char*               nameString = "Deuce";
    json_object_object_get_ex(root, "Bans", &array);

    uint8_t banned = 0;
    for (uint8_t ID = 0; ID < server->protocol.max_players; ++ID) {
        if (server->player[ID].state != STATE_DISCONNECTED && server->player[ID].ip.ip32 == ip.ip32) {
            if (banned == 0) {
                nameString = server->player[ID].name;
                banned     = 1; // Do not add multiples of the same IP. Its pointless.
            }
            enet_peer_disconnect(server->player[ID].peer, REASON_BANNED);
        }
    }
    json_object_object_add(ban, "Name", json_object_new_string(nameString));
    json_object_object_add(ban, "IP", json_object_new_string(ipString));
    json_object_object_add(ban, "Time", json_object_new_double(time));
    if (*reason == 32 && strlen(++reason) > 0) {
        json_object_object_add(ban, "Reason", json_object_new_string(reason));
    }
    json_object_array_add(array, ban);
    json_object_to_file("Bans.json", root);
    json_object_put(root);
    if (time == 0) {
        send_server_notice(
        server, arguments.player_id, arguments.console, "IP %s has been permanently banned", ipString);
    } else {
        send_server_notice(
        server, arguments.player_id, arguments.console, "IP %s has been for %f minutes", ipString, time);
    }
}

static void _cmd_admin(void* p_server, command_args_t arguments)
{
    server_t* server = (server_t*) p_server;
    if (arguments.argc > 1) {
        if (arguments.console == 0 && server->player[arguments.player_id].admin_muted == 1) {
            send_server_notice(
            server, arguments.player_id, arguments.console, "You are not allowed to use this command (Admin muted)");
            return;
        }
        if (arguments.console) {
            sendMessageToStaff(server, "Staff from %s: %s", "Console", arguments.argv[1]);
        } else {
            sendMessageToStaff(
            server, "Staff from %s: %s", server->player[arguments.player_id].name, arguments.argv[1]);
        }
        send_server_notice(server, arguments.player_id, arguments.console, "Message sent to all staff members online");
    } else {
        send_server_notice(server, arguments.player_id, arguments.console, "Invalid message");
    }
}

static void _cmd_admin_mute(void* p_server, command_args_t arguments)
{
    server_t* server = (server_t*) p_server;
    uint8_t   ID     = 33;
    if (arguments.argc == 2 && parse_player(arguments.argv[1], &ID, NULL)) {
        if (ID < server->protocol.max_players && is_past_join_screen(server, ID)) {
            if (server->player[ID].admin_muted) {
                server->player[ID].admin_muted = 0;
                send_server_notice(
                server, arguments.player_id, arguments.console, "%s has been admin unmuted", server->player[ID].name);
            } else {
                server->player[ID].admin_muted = 1;
                send_server_notice(
                server, arguments.player_id, arguments.console, "%s has been admin muted", server->player[ID].name);
            }
        } else {
            send_server_notice(
            server, arguments.player_id, arguments.console, "ID not in range or player doesnt exist");
        }
    } else {
        send_server_notice(
        server, arguments.player_id, arguments.console, "You did not enter ID or entered incorrect argument");
    }
}

static void _cmd_ban_custom(void* p_server, command_args_t arguments)
{
    server_t* server = (server_t*) p_server;
    ip_t      ip;
    char*     reason;
    uint8_t   ID;
    uint8_t   customBan = 0;
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
                _cmd_generate_ban(
                server, arguments, ((long double) get_nanos() / (uint64_t) NANO_IN_MINUTE) + time, ip, reason);
            } else {
                _cmd_generate_ban(server, arguments, time, ip, reason);
            }
        } else if (parse_player(arguments.argv[1], &ID, &reason)) {
            if (server->player[ID].state == STATE_DISCONNECTED || ID > 31) {
                send_server_notice(
                server, arguments.player_id, arguments.console, "Player with ID %hhu does not exist");
            }
            if (customBan) {
                char* temp = reason;
                parse_float(++temp, &time, &reason);
            }
            ip.ip32 = server->player[ID].ip.ip32;
            ip.cidr = server->player[ID].ip.cidr;
            if (time > 0) {
                _cmd_generate_ban(
                server, arguments, ((long double) get_nanos() / (uint64_t) NANO_IN_MINUTE) + time, ip, reason);
            } else {
                _cmd_generate_ban(server, arguments, time, ip, reason);
            }
        } else {
            send_server_notice(server, arguments.player_id, arguments.console, "Invalid IP format");
        }
    } else {
        send_server_notice(
        server, arguments.player_id, arguments.console, "You did not enter IP or entered incorrect argument");
    }
}

static void _cmd_ban_range(void* p_server, command_args_t arguments)
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

            uint8_t banned = 0;
            for (uint8_t ID = 0; ID < server->protocol.max_players; ++ID) {
                if (server->player[ID].state != STATE_DISCONNECTED &&
                    octets_in_range(startRange, endRange, server->player[ID].ip))
                {
                    if (banned == 0) {
                        nameString = server->player[ID].name;
                        banned     = 1; // Do not add multiples of the same IP. Its pointless.
                    }
                    enet_peer_disconnect(server->player[ID].peer, REASON_BANNED);
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
            send_server_notice(server,
                               arguments.player_id,
                               arguments.console,
                               "IP range %s-%s has been permanently banned",
                               ipStringStart,
                               ipStringEnd);
        } else {
            send_server_notice(server, arguments.player_id, arguments.console, "Invalid IP format");
        }
    } else {
        send_server_notice(
        server, arguments.player_id, arguments.console, "You did not enter IP or entered incorrect argument");
    }
}

static void _cmd_clin(void* p_server, command_args_t arguments)
{
    server_t* server = (server_t*) p_server;
    uint8_t   ID     = arguments.player_id;

    if (arguments.argc > 2) {
        send_server_notice(server, arguments.player_id, arguments.console, "Too many arguments");
        return;
    } else if (arguments.argc != 2 && arguments.console) {
        send_server_notice(server, arguments.player_id, arguments.console, "No ID given");
        return;
    } else if (arguments.argc == 2 && !parse_player(arguments.argv[1], &ID, NULL)) {
        send_server_notice(server, arguments.player_id, arguments.console, "Invalid ID. Wrong format");
        return;
    }

    if (ID < server->protocol.max_players && is_past_join_screen(server, ID)) {
        char client[15];
        if (server->player[ID].client == 'o') {
            snprintf(client, 12, "OpenSpades");
        } else if (server->player[ID].client == 'B') {
            snprintf(client, 13, "BetterSpades");
        } else {
            snprintf(client, 7, "Voxlap");
        }
        send_server_notice(server,
                           arguments.player_id,
                           arguments.console,
                           "Player %s is running %s version %d.%d.%d on %s",
                           server->player[ID].name,
                           client,
                           server->player[ID].version_major,
                           server->player[ID].version_minor,
                           server->player[ID].version_revision,
                           server->player[ID].os_info);
    } else {
        send_server_notice(server, arguments.player_id, arguments.console, "Invalid ID. Player doesnt exist");
    }
}

static void _cmd_help(void* p_server, command_args_t arguments)
{
    server_t*  server = (server_t*) p_server;
    command_t* cmd    = NULL;
    if (arguments.console)
        send_server_notice(server, arguments.player_id, arguments.console, "Commands available to you:");

    LL_FOREACH(server->cmds_list, cmd)
    {
        if (cmd == NULL) {
            return; // Just for safety
        }
        if (player_has_permission(server, arguments.player_id, arguments.console, cmd->permissions) ||
            cmd->permissions == 0)
        {
            send_server_notice(server,
                               arguments.player_id,
                               arguments.console,
                               "[Command: %s, Description: %s]",
                               cmd->id,
                               cmd->description);
        }
    }
    if (!arguments.console)
        send_server_notice(server, arguments.player_id, arguments.console, "Commands available to you:");
}

static void _cmd_intel(void* p_server, command_args_t arguments)
{
    server_t* server          = (server_t*) p_server;
    uint8_t   sentAtLeastOnce = 0;
    for (uint8_t playerID = 0; playerID < server->protocol.max_players; ++playerID) {
        if (server->player[playerID].state != STATE_DISCONNECTED && server->player[playerID].has_intel) {
            send_server_notice(server,
                               arguments.player_id,
                               arguments.console,
                               "Player %s (#%hhu) has intel",
                               server->player[playerID].name,
                               playerID);
            sentAtLeastOnce = 1;
        }
    }
    if (sentAtLeastOnce == 0) {
        if (server->protocol.gamemode.intel_held[0]) {
            send_server_notice(
            server, arguments.player_id, arguments.console, "Intel is not being held but intel of team 0 thinks it is");
        } else if (server->protocol.gamemode.intel_held[1]) {
            send_server_notice(
            server, arguments.player_id, arguments.console, "Intel is not being held but intel of team 1 thinks it is");
        }
        send_server_notice(server, arguments.player_id, arguments.console, "Intel is not being held");
    }
}

static void _cmd_inv(void* p_server, command_args_t arguments)
{
    server_t* server = (server_t*) p_server;
    if (arguments.console) {
        LOG_INFO("You cannot use this command from console");
        return;
    }
    if (server->player[arguments.player_id].is_invisible == 1) {
        server->player[arguments.player_id].is_invisible  = 0;
        server->player[arguments.player_id].allow_killing = 1;
        for (uint8_t i = 0; i < server->protocol.max_players; ++i) {
            if (is_past_join_screen(server, i) && i != arguments.player_id) {
                send_create_player(server, i, arguments.player_id);
            }
        }
        send_server_notice(server, arguments.player_id, arguments.console, "You are no longer invisible");
    } else if (server->player[arguments.player_id].is_invisible == 0) {
        server->player[arguments.player_id].is_invisible  = 1;
        server->player[arguments.player_id].allow_killing = 0;
        send_kill_action_packet(server, arguments.player_id, arguments.player_id, 0, 0, 1);
        send_server_notice(server, arguments.player_id, arguments.console, "You are now invisible");
    }
}

static void _cmd_kick(void* p_server, command_args_t arguments)
{
    server_t* server = (server_t*) p_server;
    uint8_t   ID     = 33;
    if (arguments.argc == 2 && parse_player(arguments.argv[1], &ID, NULL)) {
        if (ID < server->protocol.max_players && is_past_join_screen(server, ID)) {
            enet_peer_disconnect(server->player[ID].peer, REASON_KICKED);
            broadcast_server_notice(
            server, arguments.console, "Player %s (#%hhu) has been kicked", server->player[ID].name, ID);
        } else {
            send_server_notice(
            server, arguments.player_id, arguments.console, "ID not in range or player doesnt exist");
        }
    } else {
        send_server_notice(
        server, arguments.player_id, arguments.console, "You did not enter ID or entered incorrect argument");
    }
}

static void _cmd_kill(void* p_server, command_args_t arguments)
{
    server_t* server = (server_t*) p_server;
    if (arguments.argc == 1) {
        if (arguments.console) {
            LOG_INFO("You cannot use this command without arguments from console");
            return;
        }
        send_kill_action_packet(server, arguments.player_id, arguments.player_id, 0, 5, 0);
    } else {
        if (!player_has_permission(server, arguments.player_id, arguments.console, 30)) {
            send_server_notice(
            server, arguments.player_id, arguments.console, "You have no permission to use this command.");
            return;
        }
        if (strcmp(arguments.argv[1], "all") == 0) { // KILL THEM ALL!!!! >:D
            int count = 0;
            for (int i = 0; i < server->protocol.max_players; ++i) {
                if (is_past_join_screen(server, i) && server->player[i].hp > 0 &&
                    server->player[i].team != TEAM_SPECTATOR)
                {
                    send_kill_action_packet(server, i, i, 0, 5, 0);
                    count++;
                }
            }
            send_server_notice(server, arguments.player_id, arguments.console, "Killed %i players.", count);
            return;
        }

        uint8_t ID = 0;
        for (uint32_t i = 1; i < arguments.argc; i++) {
            if (!parse_player(arguments.argv[i], &ID, NULL) || ID > 32) {
                send_server_notice(
                server, arguments.player_id, arguments.console, "Invalid player \"%s\"!", arguments.argv[i]);
                return;
            }
            if (is_past_join_screen(server, ID) && server->player[i].team != TEAM_SPECTATOR) {
                send_kill_action_packet(server, ID, ID, 0, 5, 0);
                send_server_notice(server, arguments.player_id, arguments.console, "Killing player #%i...", ID);
            } else {
                send_server_notice(
                server, arguments.player_id, arguments.console, "Player %hhu does not exist or is already dead", ID);
            }
        }
    }
}

static void _cmd_login(void* p_server, command_args_t arguments)
{
    server_t* server = (server_t*) p_server;
    if (arguments.console) {
        LOG_INFO("You cannot use this command from console");
        return;
    }
    if (server->player[arguments.player_id].permissions == 0) {
        if (arguments.argc == 3) {
            char*   level    = arguments.argv[1];
            char*   password = arguments.argv[2];
            uint8_t levelLen = strlen(level);
            for (uint8_t j = 0; j < levelLen; ++j) {
                level[j] = tolower(level[j]);
            }
            uint8_t failed = 0;
            for (unsigned long i = 0; i < sizeof(server->player[arguments.player_id].role_list) / sizeof(permissions_t);
                 ++i)
            {
                if (strcmp(level, server->player[arguments.player_id].role_list[i].access_level) == 0) {
                    if (*server->player[arguments.player_id].role_list[i].access_password[0] !=
                        '\0' // disable the role if no password is set
                        && strcmp(password, *server->player[arguments.player_id].role_list[i].access_password) == 0)
                    {
                        server->player[arguments.player_id].permissions |=
                        1 << server->player[arguments.player_id].role_list[i].perm_level_offset;
                        send_server_notice(server,
                                           arguments.player_id,
                                           arguments.console,
                                           "You logged in as %s",
                                           server->player[arguments.player_id].role_list[i].access_level);
                        return;
                    } else {
                        send_server_notice(server, arguments.player_id, arguments.console, "Wrong password");
                        return;
                    }
                } else {
                    failed++;
                }
            }
            if (failed >= sizeof(server->player[arguments.player_id].role_list) / sizeof(permissions_t)) {
                send_server_notice(server, arguments.player_id, arguments.console, "Invalid role");
            }
        } else {
            send_server_notice(
            server, arguments.player_id, arguments.console, "Incorrect number of arguments to login");
        }
    } else {
        send_server_notice(server, arguments.player_id, arguments.console, "You are already logged in");
    }
}

static void _cmd_logout(void* p_server, command_args_t arguments)
{
    server_t* server = (server_t*) p_server;
    if (arguments.console) {
        LOG_INFO("You cannot use this command from console");
        return;
    }
    server->player[arguments.player_id].permissions = 0;
    send_server_notice(server, arguments.player_id, arguments.console, "You logged out");
}

static void _cmd_master(void* p_server, command_args_t arguments)
{
    server_t* server = (server_t*) p_server;
    if (server->master.enable_master_connection == 1) {
        server->master.enable_master_connection = 0;
        enet_host_destroy(server->master.client);
        send_server_notice(server, arguments.player_id, arguments.console, "Disabling master connection");
        return;
    }
    server->master.enable_master_connection = 1;
    master_connect(server, server->port);
    send_server_notice(server, arguments.player_id, arguments.console, "Enabling master connection");
}

static void _cmd_mute(void* p_server, command_args_t arguments)
{
    server_t* server = (server_t*) p_server;
    uint8_t   ID     = 33;
    if (arguments.argc == 2 && parse_player(arguments.argv[1], &ID, NULL)) {
        if (ID < server->protocol.max_players && is_past_join_screen(server, ID)) {
            if (server->player[ID].muted) {
                server->player[ID].muted = 0;
                broadcast_server_notice(server, arguments.console, "%s has been unmuted", server->player[ID].name);
            } else {
                server->player[ID].muted = 1;
                broadcast_server_notice(server, arguments.console, "%s has been muted", server->player[ID].name);
            }
        } else {
            send_server_notice(
            server, arguments.player_id, arguments.console, "ID not in range or player doesnt exist");
        }
    } else {
        send_server_notice(
        server, arguments.player_id, arguments.console, "You did not enter ID or entered incorrect argument");
    }
}

static void _cmd_pm(void* p_server, command_args_t arguments)
{
    server_t* server = (server_t*) p_server;
    char*     PM;
    uint8_t   ID = 33;
    if (arguments.argc == 2 && parse_player(arguments.argv[1], &ID, &PM) && strlen(++PM) > 0) {
        if (ID < server->protocol.max_players && is_past_join_screen(server, ID)) {
            if (arguments.console) {
                send_server_notice(server, ID, 0, "PM from Console: %s", PM);
            } else {
                send_server_notice(server, ID, 0, "PM from %s: %s", server->player[arguments.player_id].name, PM);
            }
            send_server_notice(
            server, arguments.player_id, arguments.console, "PM sent to %s", server->player[ID].name);
        } else {
            send_server_notice(server, arguments.player_id, arguments.console, "Invalid ID. Player doesnt exist");
        }
    } else {
        send_server_notice(server, arguments.player_id, arguments.console, "No ID or invalid message");
    }
}

static void _cmd_ratio(void* p_server, command_args_t arguments)
{
    server_t* server = (server_t*) p_server;
    uint8_t   ID     = 33;
    if (arguments.argc == 2 && parse_player(arguments.argv[1], &ID, NULL)) {
        if (ID < server->protocol.max_players && is_past_join_screen(server, ID)) {
            send_server_notice(server,
                               arguments.player_id,
                               arguments.console,
                               "%s has kill to death ratio of: %f (Kills: %d, Deaths: %d)",
                               server->player[ID].name,
                               ((float) server->player[ID].kills / fmaxf(1, (float) server->player[ID].deaths)),
                               server->player[ID].kills,
                               server->player[ID].deaths);
        } else {
            send_server_notice(
            server, arguments.player_id, arguments.console, "ID not in range or player doesnt exist");
        }
    } else {
        if (arguments.console) {
            send_server_notice(server,
                               arguments.player_id,
                               arguments.console,
                               "You cannot use this command from console without argument");
            return;
        }
        send_server_notice(server,
                           arguments.player_id,
                           arguments.console,
                           "%s has kill to death ratio of: %f (Kills: %d, Deaths: %d)",
                           server->player[arguments.player_id].name,
                           ((float) server->player[arguments.player_id].kills /
                            fmaxf(1, (float) server->player[arguments.player_id].deaths)),
                           server->player[arguments.player_id].kills,
                           server->player[arguments.player_id].deaths);
    }
}

static void _cmd_reset(void* p_server, command_args_t arguments)
{
    server_t* server = (server_t*) p_server;
    (void) arguments;
    for (uint32_t i = 0; i < server->protocol.max_players; ++i) {
        if (server->player[i].state != STATE_DISCONNECTED) {
            server->player[i].state = STATE_STARTING_MAP;
        }
    }
    server_reset(server);
}

static void _cmd_say(void* p_server, command_args_t arguments)
{
    server_t* server = (server_t*) p_server;
    if (arguments.argc == 2) {
        for (uint8_t ID = 0; ID < server->protocol.max_players; ++ID) {
            if (is_past_join_screen(server, ID)) {
                send_server_notice(server, ID, 0, arguments.argv[1]);
            }
        }
    } else {
        send_server_notice(server, arguments.player_id, arguments.console, "Invalid message");
    }
}

static void _cmd_server(void* p_server, command_args_t arguments)
{
    server_t* server = (server_t*) p_server;
    send_server_notice(
    server, arguments.player_id, arguments.console, "You are playing on SpadesX server. Version %s", VERSION);
}

static void _cmd_toggle_build(void* p_server, command_args_t arguments)
{
    server_t* server = (server_t*) p_server;
    uint8_t   ID;
    if (arguments.argc == 1) {
        if (server->global_ab == 1) {
            server->global_ab = 0;
            broadcast_server_notice(server, arguments.console, "Building has been disabled");
        } else if (server->global_ab == 0) {
            server->global_ab = 1;
            broadcast_server_notice(server, arguments.console, "Building has been enabled");
        }
    } else if (parse_player(arguments.argv[1], &ID, NULL)) {
        if (ID < server->protocol.max_players && is_past_join_screen(server, ID)) {
            if (server->player[ID].can_build == 1) {
                server->player[ID].can_build = 0;
                broadcast_server_notice(
                server, arguments.console, "Building has been disabled for %s", server->player[ID].name);
            } else if (server->player[ID].can_build == 0) {
                server->player[ID].can_build = 1;
                broadcast_server_notice(
                server, arguments.console, "Building has been enabled for %s", server->player[ID].name);
            }
        } else {
            send_server_notice(
            server, arguments.player_id, arguments.console, "ID not in range or player doesnt exist");
        }
    }
}

static void _cmd_toggle_kill(void* p_server, command_args_t arguments)
{
    server_t* server = (server_t*) p_server;
    uint8_t   id;
    if (arguments.argc == 1) {
        if (server->global_ak == 1) {
            server->global_ak = 0;
            broadcast_server_notice(server, arguments.console, "Killing has been disabled");
        } else if (server->global_ak == 0) {
            server->global_ak = 1;
            broadcast_server_notice(server, arguments.console, "Killing has been enabled");
        }
    } else if (parse_player(arguments.argv[1], &id, NULL)) {
        if (id < server->protocol.max_players && is_past_join_screen(server, id)) {
            if (server->player[id].allow_killing == 1) {
                server->player[id].allow_killing = 0;
                broadcast_server_notice(
                server, arguments.console, "Killing has been disabled for %s", server->player[id].name);
            } else if (server->player[id].allow_killing == 0) {
                server->player[id].allow_killing = 1;
                broadcast_server_notice(
                server, arguments.console, "Killing has been enabled for %s", server->player[id].name);
            }
        } else {
            send_server_notice(
            server, arguments.player_id, arguments.console, "ID not in range or player doesnt exist");
        }
    }
}

static void _cmd_toggle_team_kill(void* p_server, command_args_t arguments)
{
    server_t* server = (server_t*) p_server;
    uint8_t   ID     = 33;
    if (arguments.argc == 2 && parse_player(arguments.argv[1], &ID, NULL)) {
        if (ID < server->protocol.max_players && is_past_join_screen(server, ID)) {
            if (server->player[ID].allow_team_killing) {
                server->player[ID].allow_team_killing = 0;
                broadcast_server_notice(
                server, arguments.console, "Team killing has been disabled for %s", server->player[ID].name);
            } else {
                server->player[ID].allow_team_killing = 1;
                broadcast_server_notice(
                server, arguments.console, "Team killing has been enabled for %s", server->player[ID].name);
            }
        } else {
            send_server_notice(
            server, arguments.player_id, arguments.console, "ID not in range or player doesnt exist");
        }
    } else {
        send_server_notice(
        server, arguments.player_id, arguments.console, "You did not enter ID or entered incorrect argument");
    }
}

static void _cmd_tp(void* p_server, command_args_t arguments)
{
    server_t* server = (server_t*) p_server;
    if (arguments.console) {
        LOG_INFO("You cannot use this command from console");
        return;
    }
    uint8_t player_to_be_teleported = 33;
    uint8_t player_to_teleport_to   = 33;
    if (arguments.argc == 3 && parse_player(arguments.argv[1], &player_to_be_teleported, NULL) &&
        parse_player(arguments.argv[2], &player_to_teleport_to, NULL))
    {
        if (valid_pos_v3f(server, server->player[player_to_teleport_to].movement.position)) {
            movement_t* from = &server->player[player_to_be_teleported].movement;
            movement_t* to   = &server->player[player_to_teleport_to].movement;
            from->position.x = to->position.x - 0.5f;
            from->position.y = to->position.y - 0.5f;
            from->position.z = to->position.z - 2.36f;
            send_position_packet(server, player_to_be_teleported, from->position.x, from->position.y, from->position.z);
        } else {
            send_server_notice(server,
                               arguments.player_id,
                               arguments.console,
                               "Player %hhu is at invalid position",
                               player_to_teleport_to);
        }
    } else {
        send_server_notice(
        server, arguments.player_id, arguments.console, "Incorrect amount of arguments or wrong argument type");
    }
}

static void _cmd_tpc(void* p_server, command_args_t arguments)
{
    server_t* server = (server_t*) p_server;
    if (arguments.console) {
        LOG_INFO("You cannot use this command from console");
        return;
    }
    vector3f_t pos = {0, 0, 0};
    if (arguments.argc == 4 && PARSE_VECTOR3F(arguments, 1, &pos)) {
        if (valid_pos_v3f(server, pos)) {
            server->player[arguments.player_id].movement.position.x = pos.x - 0.5f;
            server->player[arguments.player_id].movement.position.y = pos.y - 0.5f;
            server->player[arguments.player_id].movement.position.z = pos.z - 2.36f;
            send_position_packet(server,
                                 arguments.player_id,
                                 server->player[arguments.player_id].movement.position.x,
                                 server->player[arguments.player_id].movement.position.y,
                                 server->player[arguments.player_id].movement.position.z);
        } else {
            send_server_notice(server, arguments.player_id, arguments.console, "Invalid position");
        }
    } else {
        send_server_notice(
        server, arguments.player_id, arguments.console, "Incorrect amount of arguments or wrong argument type");
    }
}

static void _cmd_unban(void* p_server, command_args_t arguments)
{
    server_t* server = (server_t*) p_server;
    ip_t      ip;
    uint8_t   unbanned = 0;
    if (arguments.argc == 2 && parse_ip(arguments.argv[1], &ip, NULL)) {
        struct json_object* array;
        struct json_object* root = json_object_from_file("Bans.json");
        json_object_object_get_ex(root, "Bans", &array);
        int         count = json_object_array_length(array);
        const char* IPString;
        char        unbanIPString[19];
        format_ip_to_str(unbanIPString, ip);
        for (int i = 0; i < count; ++i) {
            struct json_object* objectAtIndex = json_object_array_get_idx(array, i);
            READ_STR_FROM_JSON(objectAtIndex, IPString, IP, "IP", "0.0.0.0", 1);
            if (strcmp(unbanIPString, IPString) == 0) {
                json_object_array_del_idx(array, i, 1);
                json_object_to_file("Bans.json", root);
                unbanned = 1;
            }
        }
        if (unbanned) {
            send_server_notice(server, arguments.player_id, arguments.console, "IP %s unbanned", IPString);
        } else {
            send_server_notice(
            server, arguments.player_id, arguments.console, "IP %s not found in banned IP list", unbanIPString);
        }
    } else {
        send_server_notice(
        server, arguments.player_id, arguments.console, "Incorrect amount of arguments or invalid IP");
    }
}

static void _cmd_unban_range(void* p_server, command_args_t arguments)
{
    server_t* server = (server_t*) p_server;
    ip_t      start_range, end_range;
    char*     end;
    uint8_t   unbanned = 0;
    if (arguments.argc == 2 && parse_ip(arguments.argv[1], &start_range, &end) && parse_ip(end, &end_range, NULL)) {
        struct json_object* array;
        struct json_object* root = json_object_from_file("Bans.json");
        json_object_object_get_ex(root, "Bans", &array);
        int         count = json_object_array_length(array);
        const char* start_ip_string;
        const char* end_ip_string;
        char        unban_start_range_string[19];
        char        unban_end_range_string[19];
        format_ip_to_str(unban_start_range_string, start_range);
        format_ip_to_str(unban_end_range_string, end_range);
        for (int i = 0; i < count; ++i) {
            struct json_object* object_at_index = json_object_array_get_idx(array, i);
            READ_STR_FROM_JSON(object_at_index, start_ip_string, start_of_range, "start of range", "0.0.0.0", 0);
            READ_STR_FROM_JSON(object_at_index, end_ip_string, end_of_range, "end of range", "0.0.0.0", 0);
            if (strcmp(unban_start_range_string, start_ip_string) == 0 &&
                strcmp(unban_end_range_string, end_ip_string) == 0)
            {
                json_object_array_del_idx(array, i, 1);
                json_object_to_file("Bans.json", root);
                unbanned = 1;
            }
        }
        if (unbanned) {
            send_server_notice(
            server, arguments.player_id, arguments.console, "IP range %s-%s unbanned", start_ip_string, end_ip_string);
        } else {
            send_server_notice(server,
                               arguments.player_id,
                               arguments.console,
                               "IP range %s-%s not found in banned IP ranges",
                               unban_start_range_string,
                               unban_end_range_string);
        }
    } else {
        send_server_notice(
        server, arguments.player_id, arguments.console, "Incorrect amount of arguments or invalid IP");
    }
}

static void _cmd_undo_ban(void* p_server, command_args_t arguments)
{
    server_t*   server = (server_t*) p_server;
    const char* ip_string;
    const char* start_ip;
    const char* end_ip;
    if (arguments.argc == 1) {
        struct json_object* array;
        struct json_object* root = json_object_from_file("Bans.json");
        json_object_object_get_ex(root, "Bans", &array);
        int                 count         = json_object_array_length(array);
        struct json_object* objectAtIndex = json_object_array_get_idx(array, count - 1);
        READ_STR_FROM_JSON(objectAtIndex, ip_string, IP, "IP", "0.0.0.0", 1);
        if (ip_string[0] == '0') {
            READ_STR_FROM_JSON(objectAtIndex, start_ip, start_of_range, "start of range", "0.0.0.0", 0);
            READ_STR_FROM_JSON(objectAtIndex, end_ip, end_of_range, "end of range", "0.0.0.0", 0);
            json_object_array_del_idx(array, count - 1, 1);
            json_object_to_file("Bans.json", root);
            send_server_notice(
            server, arguments.player_id, arguments.console, "IP range %s-%s unbanned", start_ip, end_ip);
        } else {
            json_object_array_del_idx(array, count - 1, 1);
            json_object_to_file("Bans.json", root);
            send_server_notice(server, arguments.player_id, arguments.console, "IP %s unbanned", ip_string);
        }
    } else {
        send_server_notice(server, arguments.player_id, arguments.console, "Too many arguments given to command");
    }
}

static void _cmd_ups(void* p_server, command_args_t arguments)
{
    server_t* server = (server_t*) p_server;
    if (arguments.console) {
        LOG_INFO("You cannot use this command from console");
        return;
    }
    if (arguments.argc != 2) {
        return;
    }
    float ups = 0;
    parse_float(arguments.argv[1], &ups, NULL);
    if (ups >= 10 && ups <= 300) {
        server->player[arguments.player_id].ups = ups;
        send_server_notice(server, arguments.player_id, arguments.console, "UPS changed to %.2f successfully", ups);
    } else {
        send_server_notice(
        server, arguments.player_id, arguments.console, "Changing UPS failed. Please select value between 1 and 300");
    }
}

void command_create(server_t* server,
                    uint8_t   parse_args,
                    void (*command)(void* p_server, command_args_t arguments),
                    char     id[30],
                    char     description[1024],
                    uint32_t permissions)
{
    command_t* cmd   = malloc(sizeof(command_t));
    cmd->execute     = command;
    cmd->parse_args  = parse_args;
    cmd->permissions = permissions;
    strcpy(cmd->description, description);
    strcpy(cmd->id, id);
    HASH_ADD_STR(server->cmds_map, id, cmd);
    LL_APPEND(server->cmds_list, cmd);
}

void command_populate_all(server_t* server)
{
    server->cmds_list = NULL; // UTlist requires root to be initialized to NULL
    //  Add custom commands into this array definition
    command_manager_t commands[] = {
    {"/admin", 0, &_cmd_admin, 0, "Sends a message to all online admins."},
    {"/adminmute", 1, &_cmd_admin_mute, 30, "Mutes or unmutes player from /admin usage"},
    {"/ban", 0, &_cmd_ban_custom, 30, "Puts specified IP into ban list"},
    {"/banrange", 0, &_cmd_ban_range, 30, "Bans specified IP range"},
    // We can have 2+ commands for same function even with different permissions and name
    {"/client", 1, &_cmd_clin, 0, "Shows players client info"},
    {"/clin", 1, &_cmd_clin, 0, "Shows players client info"},
    {"/dban", 0, &_cmd_ban_custom, 30, "Bans specified player for a day"},
    {"/hban", 0, &_cmd_ban_custom, 30, "Bans specified player for 6 hours"},
    {"/help", 0, &_cmd_help, 0, "Shows commands and their description"},
    {"/intel", 0, &_cmd_intel, 0, "Shows info about intel"},
    {"/inv", 0, &_cmd_inv, 30, "Makes you go invisible"},
    {"/kick", 1, &_cmd_kick, 30, "Kicks specified player from the server"},
    {"/kill", 1, &_cmd_kill, 0, "Kills player who sent it or player specified in argument"},
    {"/login", 1, &_cmd_login, 0, "Login command. First argument is a role. Second password"},
    {"/logout", 0, &_cmd_logout, 31, "Logs out logged in player"},
    {"/master", 0, &_cmd_master, 28, "Toggles master connection"},
    {"/mban", 0, &_cmd_ban_custom, 30, "Bans specified player for a month"},
    {"/mute", 1, &_cmd_mute, 30, "Mutes or unmutes specified player"},
    {"/pban", 0, &_cmd_ban_custom, 30, "Permanently bans a specified player"},
    {"/pm", 0, &_cmd_pm, 0, "Private message to specified player"},
    {"/ratio", 1, &_cmd_ratio, 0, "Shows yours or requested player ratio"},
    {"/reset", 0, &_cmd_reset, 24, "Resets server and loads next map"},
    {"/say", 0, &_cmd_say, 30, "Send message to everyone as the server"},
    {"/server", 0, &_cmd_server, 0, "Shows info about the server"},
    {"/tb", 1, &_cmd_toggle_build, 30, "Toggles ability to build for everyone or specified player"},
    {"/tk", 1, &_cmd_toggle_kill, 30, "Toggles ability to kill for everyone or specified player"},
    {"/tp", 1, &_cmd_tp, 24, "Teleports specified player to another specified player"},
    {"/tpc", 1, &_cmd_tpc, 24, "Teleports to specified cordinates"},
    {"/ttk", 1, &_cmd_toggle_team_kill, 30, "Toggles ability to team kill for everyone or specified player"},
    {"/unban", 0, &_cmd_unban, 30, "Unbans specified IP"},
    {"/unbanrange", 0, &_cmd_unban_range, 30, "Unbans specified IP range"},
    {"/undoban", 0, &_cmd_undo_ban, 30, "Reverts the last ban"},
    {"/ups", 1, &_cmd_ups, 0, "Sets UPS of player to requested ammount. Range: 1-300"},
    {"/wban", 0, &_cmd_ban_custom, 30, "Bans specified player for a week"}};
    for (unsigned long i = 0; i < sizeof(commands) / sizeof(command_manager_t); i++) {
        command_create(server,
                       commands[i].parse_args,
                       commands[i].command,
                       commands[i].id,
                       commands[i].description,
                       commands[i].permissions);
    }
    LL_SORT(server->cmds_list, _cmds_compare);
}

void command_free_all(server_t* server)
{
    command_t* current_command;
    command_t* tmp;

    HASH_ITER(hh, server->cmds_map, current_command, tmp)
    {
        command_free(server, current_command);
    }
}

void command_free(server_t* server, command_t* command)
{
    HASH_DEL(server->cmds_map, command);
    LL_DELETE(server->cmds_list, command);
    free(command);
}

static uint8_t parse_arguments(server_t* server, command_args_t* arguments, char* message, uint8_t commandLength)
{
    char* p = message + commandLength; // message beginning + command length
    while (*p != '\0' && arguments->argc < 32) {
        uint8_t escaped = 0, quotesCount = 0, argument_length;
        while (*p == ' ' || *p == '\t')
            p++; // rewinding
        if (*p == '\0')
            break;
        char* end = p;
        if (*end == '"') {
            quotesCount++;
            p++;
            end++;
        }
        while (*end) {
            if (escaped) {
                escaped = 0;
            } else {
                switch (*end) {
                    case ' ':
                    case '\t':
                        if (quotesCount == 0) {
                            goto argparse_loop_exit;
                        }
                        break;
                    case '\\':
                        escaped = 1;
                        break;
                    case '"':
                        if (quotesCount == 0) {
                            send_server_notice(server,
                                               arguments->player_id,
                                               arguments->console,
                                               "Failed to parse the command: found a stray \" symbol");
                            return 0;
                        }
                        char next = *(end + 1);
                        if (next != ' ' && next != '\t' && next != '\0') {
                            send_server_notice(server,
                                               arguments->player_id,
                                               arguments->console,
                                               "Failed to parse the command: found more symbols after the \" symbol");
                            return 0;
                        }
                        quotesCount++;
                        end++;
                        goto argparse_loop_exit;
                        break;
                    default:
                        break;
                }
            }
            end++;
        }
        if (quotesCount == 1) {
            send_server_notice(
            server, arguments->player_id, arguments->console, "Failed to parse the command: missing a \" symbol");
            return 0;
        }
    argparse_loop_exit:
        argument_length = end - p;
        if (quotesCount == 2) {
            argument_length--; // don't need that last quote mark
        }
        if (argument_length) {
            char* argument            = malloc(argument_length + 1); // Don't forget about the null terminator!
            argument[argument_length] = '\0';
            memcpy(argument, p, argument_length);
            arguments->argv[arguments->argc++] = argument;
        }
        p = end;
    }
    return 1;
}

void command_handle(server_t* server, uint8_t player_id, char* message, uint8_t console)
{
    char* command = calloc(1000, sizeof(uint8_t));
    sscanf(message, "%s", command);
    uint8_t command_length = strlen(command);
    for (uint8_t i = 1; i < command_length; ++i) {
        message[i] = command[i] = tolower(command[i]);
    }

    command_t* cmd;
    HASH_FIND_STR(server->cmds_map, command, cmd);
    if (cmd == NULL) {
        free(command);
        return;
    }

    command_args_t arguments;
    arguments.player_id   = player_id;
    arguments.permissions = cmd->permissions;
    arguments.console     = console;
    arguments.argv[0]     = command;
    arguments.argc        = 1;

    uint8_t message_length;
    if (cmd->parse_args) {
        if (!parse_arguments(server, &arguments, message, command_length)) {
            goto epic_parsing_fail;
        }
    } else if ((message_length = strlen(message)) - command_length > 1)
    { // if we have something other than the command itself
        char* argument = calloc(message_length - command_length, sizeof(message[0]));
        strcpy(argument, message + command_length + 1);
        arguments.argv[arguments.argc++] = argument;
    }

    if (player_has_permission(server, player_id, console, cmd->permissions) > 0 || cmd->permissions == 0) {
        cmd->execute((void*) server, arguments);
    } else {
        send_server_notice(server, player_id, console, "You do not have permissions to use this command");
    }
epic_parsing_fail:
    for (uint8_t i = 1; i < arguments.argc; i++) { // Starting from 1 since we'll free the command anyway
        free(arguments.argv[i]);
    }
    free(command);
}
