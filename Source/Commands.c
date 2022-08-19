#include "Commands.h"

#include "Master.h"
#include "Packets.h"
#include "ParseConvert.h"
#include "Protocol.h"
#include "Server.h"
#include "Structs.h"
#include "Util/Enums.h"
#include "Util/JSONHelpers.h"
#include "Util/Types.h"
#include "Util/Uthash.h"
#include "Util/Utlist.h"
#include "Util/Log.h"

#include <ctype.h>
#include <enet/enet.h>
#include <json-c/json.h>
#include <json-c/json_object.h>
#include <json-c/json_util.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

static uint32 commandCompare(Command* first, Command* second)
{
    return strcmp(first->id, second->id);
}

static void genBan(Server* server, CommandArguments arguments, float time, IPStruct ip, char* reason)
{
    char ipString[19];
    formatIPToString(ipString, ip);
    struct json_object* array;
    struct json_object* ban        = json_object_new_object();
    struct json_object* root       = json_object_from_file("Bans.json");
    char*               nameString = "Deuce";
    json_object_object_get_ex(root, "Bans", &array);

    uint8 banned = 0;
    for (uint8 ID = 0; ID < server->protocol.maxPlayers; ++ID) {
        if (server->player[ID].state != STATE_DISCONNECTED && server->player[ID].ipStruct.Union.ip32 == ip.Union.ip32) {
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
        sendServerNotice(server, arguments.player, arguments.console, "IP %s has been permanently banned", ipString);
    } else {
        sendServerNotice(server, arguments.player, arguments.console, "IP %s has been for %f minutes", ipString, time);
    }
}

static void adminCommand(void* serverP, CommandArguments arguments)
{
    Server* server = (Server*) serverP;
    if (arguments.argc > 1) {
        if (arguments.console == 0 && server->player[arguments.player].adminMuted == 1) {
            sendServerNotice(
            server, arguments.player, arguments.console, "You are not allowed to use this command (Admin muted)");
            return;
        }
        if (arguments.console) {
            sendMessageToStaff(server, "Staff from %s: %s", "Console", arguments.argv[1]);
        } else {
            sendMessageToStaff(server, "Staff from %s: %s", server->player[arguments.player].name, arguments.argv[1]);
        }
        sendServerNotice(server, arguments.player, arguments.console, "Message sent to all staff members online");
    } else {
        sendServerNotice(server, arguments.player, arguments.console, "Invalid message");
    }
}

static void adminMuteCommand(void* serverP, CommandArguments arguments)
{
    Server* server = (Server*) serverP;
    uint8   ID     = 33;
    if (arguments.argc == 2 && parsePlayer(arguments.argv[1], &ID, NULL)) {
        if (ID < server->protocol.maxPlayers && isPastJoinScreen(server, ID)) {
            if (server->player[ID].adminMuted) {
                server->player[ID].adminMuted = 0;
                sendServerNotice(
                server, arguments.player, arguments.console, "%s has been admin unmuted", server->player[ID].name);
            } else {
                server->player[ID].adminMuted = 1;
                sendServerNotice(
                server, arguments.player, arguments.console, "%s has been admin muted", server->player[ID].name);
            }
        } else {
            sendServerNotice(server, arguments.player, arguments.console, "ID not in range or player doesnt exist");
        }
    } else {
        sendServerNotice(
        server, arguments.player, arguments.console, "You did not enter ID or entered incorrect argument");
    }
}

static void banCustomCommand(void* serverP, CommandArguments arguments)
{
    Server*  server = (Server*) serverP;
    IPStruct ip;
    char*    reason;
    uint8    ID;
    uint8    customBan = 0;
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

        if (parseIP(arguments.argv[1], &ip, &reason)) {
            if (customBan) {
                char* temp = reason;
                parseFloat(++temp, &time, &reason);
            }
            if (time > 0) {
                genBan(server, arguments, ((long double) getNanos() / (uint64) NANO_IN_MINUTE) + time, ip, reason);
            } else {
                genBan(server, arguments, time, ip, reason);
            }
        } else if (parsePlayer(arguments.argv[1], &ID, &reason)) {
            if (server->player[ID].state == STATE_DISCONNECTED || ID > 31) {
                sendServerNotice(server, arguments.player, arguments.console, "Player with ID %hhu does not exist");
            }
            if (customBan) {
                char* temp = reason;
                parseFloat(++temp, &time, &reason);
            }
            ip.Union.ip32 = server->player[ID].ipStruct.Union.ip32;
            ip.CIDR       = server->player[ID].ipStruct.CIDR;
            if (time > 0) {
                genBan(server, arguments, ((long double) getNanos() / (uint64) NANO_IN_MINUTE) + time, ip, reason);
            } else {
                genBan(server, arguments, time, ip, reason);
            }
        } else {
            sendServerNotice(server, arguments.player, arguments.console, "Invalid IP format");
        }
    } else {
        sendServerNotice(
        server, arguments.player, arguments.console, "You did not enter IP or entered incorrect argument");
    }
}

static void banRangeCommand(void* serverP, CommandArguments arguments)
{
    Server*  server = (Server*) serverP;
    IPStruct startRange;
    IPStruct endRange;
    char*    reason;
    char*    end;
    if (arguments.argc == 2) {
        if (parseIP(arguments.argv[1], &startRange, &end) && parseIP(++end, &endRange, &reason)) {
            char* temp = reason;
            float time = 0.0f;
            parseFloat(++temp, &time, &reason);
            char ipStringStart[16];
            char ipStringEnd[16];
            formatIPToString(ipStringStart, startRange); // Reformatting the IP to avoid stuff like 001.02.3.4
            formatIPToString(ipStringEnd, endRange);
            struct json_object* array;
            struct json_object* ban        = json_object_new_object();
            struct json_object* root       = json_object_from_file("Bans.json");
            char*               nameString = "Deuce";
            json_object_object_get_ex(root, "Bans", &array);

            uint8 banned = 0;
            for (uint8 ID = 0; ID < server->protocol.maxPlayers; ++ID) {
                if (server->player[ID].state != STATE_DISCONNECTED &&
                    octetsInRange(startRange, endRange, server->player[ID].ipStruct)) {
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
                ban, "Time", json_object_new_double(((long double) getNanos() / (uint64) NANO_IN_MINUTE) + time));
            } else {
                json_object_object_add(ban, "Time", json_object_new_double(time));
            }

            if (*reason == 32 && strlen(++reason) > 0) {
                json_object_object_add(ban, "Reason", json_object_new_string(reason));
            }
            json_object_array_add(array, ban);
            json_object_to_file("Bans.json", root);
            json_object_put(root);
            sendServerNotice(server,
                             arguments.player,
                             arguments.console,
                             "IP range %s-%s has been permanently banned",
                             ipStringStart,
                             ipStringEnd);
        } else {
            sendServerNotice(server, arguments.player, arguments.console, "Invalid IP format");
        }
    } else {
        sendServerNotice(
        server, arguments.player, arguments.console, "You did not enter IP or entered incorrect argument");
    }
}

static void clinCommand(void* serverP, CommandArguments arguments)
{
    Server* server = (Server*) serverP;
    uint8   ID     = arguments.player;

    if (arguments.argc > 2) {
        sendServerNotice(server, arguments.player, arguments.console, "Too many arguments");
        return;
    } else if (arguments.argc != 2 && arguments.console) {
        sendServerNotice(server, arguments.player, arguments.console, "No ID given");
        return;
    } else if (arguments.argc == 2 && !parsePlayer(arguments.argv[1], &ID, NULL)) {
        sendServerNotice(server, arguments.player, arguments.console, "Invalid ID. Wrong format");
        return;
    }

    if (ID < server->protocol.maxPlayers && isPastJoinScreen(server, ID)) {
        char client[15];
        if (server->player[ID].client == 'o') {
            snprintf(client, 12, "OpenSpades");
        } else if (server->player[ID].client == 'B') {
            snprintf(client, 13, "BetterSpades");
        } else {
            snprintf(client, 7, "Voxlap");
        }
        sendServerNotice(server,
                         arguments.player,
                         arguments.console,
                         "Player %s is running %s version %d.%d.%d on %s",
                         server->player[ID].name,
                         client,
                         server->player[ID].version_major,
                         server->player[ID].version_minor,
                         server->player[ID].version_revision,
                         server->player[ID].os_info);
    } else {
        sendServerNotice(server, arguments.player, arguments.console, "Invalid ID. Player doesnt exist");
    }
}

static void helpCommand(void* serverP, CommandArguments arguments)
{
    Server*  server = (Server*) serverP;
    Command* command = NULL;
    if (arguments.console)
        sendServerNotice(server, arguments.player, arguments.console, "Commands available to you:");

    LL_FOREACH(server->commandsList, command)
    {
        if (command == NULL) {
            return; //Just for safety
        }
        if (playerHasPermission(server, arguments.player, arguments.console, command->PermLevel) ||
            command->PermLevel == 0) {
            sendServerNotice(server,
                             arguments.player,
                             arguments.console,
                             "[Command: %s, Description: %s]",
                             command->id,
                             command->commandDesc);
        }
    }
    if (!arguments.console)
        sendServerNotice(server, arguments.player, arguments.console, "Commands available to you:");
}

static void intelCommand(void* serverP, CommandArguments arguments)
{
    Server* server          = (Server*) serverP;
    uint8   sentAtLeastOnce = 0;
    for (uint8 playerID = 0; playerID < server->protocol.maxPlayers; ++playerID) {
        if (server->player[playerID].state != STATE_DISCONNECTED && server->player[playerID].hasIntel) {
            sendServerNotice(server,
                             arguments.player,
                             arguments.console,
                             "Player %s (#%hhu) has intel",
                             server->player[playerID].name,
                             playerID);
            sentAtLeastOnce = 1;
        }
    }
    if (sentAtLeastOnce == 0) {
        if (server->protocol.gameMode.intelHeld[0]) {
            sendServerNotice(
            server, arguments.player, arguments.console, "Intel is not being held but intel of team 0 thinks it is");
        } else if (server->protocol.gameMode.intelHeld[1]) {
            sendServerNotice(
            server, arguments.player, arguments.console, "Intel is not being held but intel of team 1 thinks it is");
        }
        sendServerNotice(server, arguments.player, arguments.console, "Intel is not being held");
    }
}

static void invCommand(void* serverP, CommandArguments arguments)
{
    Server* server = (Server*) serverP;
    if (arguments.console) {
        LOG_INFO("You cannot use this command from console");
        return;
    }
    if (server->player[arguments.player].isInvisible == 1) {
        server->player[arguments.player].isInvisible  = 0;
        server->player[arguments.player].allowKilling = 1;
        for (uint8 i = 0; i < server->protocol.maxPlayers; ++i) {
            if (isPastJoinScreen(server, i) && i != arguments.player) {
                sendCreatePlayer(server, i, arguments.player);
            }
        }
        sendServerNotice(server, arguments.player, arguments.console, "You are no longer invisible");
    } else if (server->player[arguments.player].isInvisible == 0) {
        server->player[arguments.player].isInvisible  = 1;
        server->player[arguments.player].allowKilling = 0;
        sendKillActionPacket(server, arguments.player, arguments.player, 0, 0, 1);
        sendServerNotice(server, arguments.player, arguments.console, "You are now invisible");
    }
}

static void kickCommand(void* serverP, CommandArguments arguments)
{
    Server* server = (Server*) serverP;
    uint8   ID     = 33;
    if (arguments.argc == 2 && parsePlayer(arguments.argv[1], &ID, NULL)) {
        if (ID < server->protocol.maxPlayers && isPastJoinScreen(server, ID)) {
            enet_peer_disconnect(server->player[ID].peer, REASON_KICKED);
            broadcastServerNotice(
            server, arguments.console, "Player %s (#%hhu) has been kicked", server->player[ID].name, ID);
        } else {
            sendServerNotice(server, arguments.player, arguments.console, "ID not in range or player doesnt exist");
        }
    } else {
        sendServerNotice(
        server, arguments.player, arguments.console, "You did not enter ID or entered incorrect argument");
    }
}

static void killCommand(void* serverP, CommandArguments arguments)
{
    Server* server = (Server*) serverP;
    if (arguments.argc == 1) {
        if (arguments.console) {
            LOG_INFO("You cannot use this command without arguments from console");
            return;
        }
        sendKillActionPacket(server, arguments.player, arguments.player, 0, 5, 0);
    } else {
        if (!playerHasPermission(server, arguments.player, arguments.console, 30)) {
            sendServerNotice(
            server, arguments.player, arguments.console, "You have no permission to use this command.");
            return;
        }
        if (strcmp(arguments.argv[1], "all") == 0) { // KILL THEM ALL!!!! >:D
            int count = 0;
            for (int i = 0; i < server->protocol.maxPlayers; ++i) {
                if (isPastJoinScreen(server, i) && server->player[i].HP > 0 && server->player[i].team != TEAM_SPECTATOR)
                {
                    sendKillActionPacket(server, i, i, 0, 5, 0);
                    count++;
                }
            }
            sendServerNotice(server, arguments.player, arguments.console, "Killed %i players.", count);
            return;
        }

        uint8 ID = 0;
        for (uint32 i = 1; i < arguments.argc; i++) {
            if (!parsePlayer(arguments.argv[i], &ID, NULL) || ID > 32) {
                sendServerNotice(
                server, arguments.player, arguments.console, "Invalid player \"%s\"!", arguments.argv[i]);
                return;
            }
            if (isPastJoinScreen(server, ID) && server->player[i].team != TEAM_SPECTATOR) {
                sendKillActionPacket(server, ID, ID, 0, 5, 0);
                sendServerNotice(server, arguments.player, arguments.console, "Killing player #%i...", ID);
            } else {
                sendServerNotice(
                server, arguments.player, arguments.console, "Player %hhu does not exist or is already dead", ID);
            }
        }
    }
}

static void loginCommand(void* serverP, CommandArguments arguments)
{
    Server* server = (Server*) serverP;
    if (arguments.console) {
        LOG_INFO("You cannot use this command from console");
        return;
    }
    if (server->player[arguments.player].permLevel == 0) {
        if (arguments.argc == 3) {
            char* level    = arguments.argv[1];
            char* password = arguments.argv[2];
            uint8 levelLen = strlen(level);
            for (uint8 j = 0; j < levelLen; ++j) {
                level[j] = tolower(level[j]);
            }
            uint8 failed = 0;
            for (unsigned long i = 0; i < sizeof(server->player[arguments.player].roleList) / sizeof(PermLevel); ++i) {
                if (strcmp(level, server->player[arguments.player].roleList[i].accessLevel) == 0) {
                    if (*server->player[arguments.player].roleList[i].accessPassword[0] !=
                        '\0' // disable the role if no password is set
                        && strcmp(password, *server->player[arguments.player].roleList[i].accessPassword) == 0)
                    {
                        server->player[arguments.player].permLevel |=
                        1 << server->player[arguments.player].roleList[i].permLevelOffset;
                        sendServerNotice(server,
                                         arguments.player,
                                         arguments.console,
                                         "You logged in as %s",
                                         server->player[arguments.player].roleList[i].accessLevel);
                        return;
                    } else {
                        sendServerNotice(server, arguments.player, arguments.console, "Wrong password");
                        return;
                    }
                } else {
                    failed++;
                }
            }
            if (failed >= sizeof(server->player[arguments.player].roleList) / sizeof(PermLevel)) {
                sendServerNotice(server, arguments.player, arguments.console, "Invalid role");
            }
        } else {
            sendServerNotice(server, arguments.player, arguments.console, "Incorrect number of arguments to login");
        }
    } else {
        sendServerNotice(server, arguments.player, arguments.console, "You are already logged in");
    }
}

static void logoutCommand(void* serverP, CommandArguments arguments)
{
    Server* server = (Server*) serverP;
    if (arguments.console) {
        LOG_INFO("You cannot use this command from console");
        return;
    }
    server->player[arguments.player].permLevel = 0;
    sendServerNotice(server, arguments.player, arguments.console, "You logged out");
}

static void masterCommand(void* serverP, CommandArguments arguments)
{
    Server* server = (Server*) serverP;
    if (server->master.enableMasterConnection == 1) {
        server->master.enableMasterConnection = 0;
        enet_host_destroy(server->master.client);
        sendServerNotice(server, arguments.player, arguments.console, "Disabling master connection");
        return;
    }
    server->master.enableMasterConnection = 1;
    ConnectMaster(server, server->port);
    sendServerNotice(server, arguments.player, arguments.console, "Enabling master connection");
}

static void muteCommand(void* serverP, CommandArguments arguments)
{
    Server* server = (Server*) serverP;
    uint8   ID     = 33;
    if (arguments.argc == 2 && parsePlayer(arguments.argv[1], &ID, NULL)) {
        if (ID < server->protocol.maxPlayers && isPastJoinScreen(server, ID)) {
            if (server->player[ID].muted) {
                server->player[ID].muted = 0;
                broadcastServerNotice(server, arguments.console, "%s has been unmuted", server->player[ID].name);
            } else {
                server->player[ID].muted = 1;
                broadcastServerNotice(server, arguments.console, "%s has been muted", server->player[ID].name);
            }
        } else {
            sendServerNotice(server, arguments.player, arguments.console, "ID not in range or player doesnt exist");
        }
    } else {
        sendServerNotice(
        server, arguments.player, arguments.console, "You did not enter ID or entered incorrect argument");
    }
}

static void pmCommand(void* serverP, CommandArguments arguments)
{
    Server* server = (Server*) serverP;
    char*   PM;
    uint8   ID = 33;
    if (arguments.argc == 2 && parsePlayer(arguments.argv[1], &ID, &PM) && strlen(++PM) > 0) {
        if (ID < server->protocol.maxPlayers && isPastJoinScreen(server, ID)) {
            if (arguments.console) {
                sendServerNotice(server, ID, 0, "PM from Console: %s", PM);
            } else {
                sendServerNotice(server, ID, 0, "PM from %s: %s", server->player[arguments.player].name, PM);
            }
            sendServerNotice(server, arguments.player, arguments.console, "PM sent to %s", server->player[ID].name);
        } else {
            sendServerNotice(server, arguments.player, arguments.console, "Invalid ID. Player doesnt exist");
        }
    } else {
        sendServerNotice(server, arguments.player, arguments.console, "No ID or invalid message");
    }
}

static void ratioCommand(void* serverP, CommandArguments arguments)
{
    Server* server = (Server*) serverP;
    uint8   ID     = 33;
    if (arguments.argc == 2 && parsePlayer(arguments.argv[1], &ID, NULL)) {
        if (ID < server->protocol.maxPlayers && isPastJoinScreen(server, ID)) {
            sendServerNotice(server,
                             arguments.player,
                             arguments.console,
                             "%s has kill to death ratio of: %f (Kills: %d, Deaths: %d)",
                             server->player[ID].name,
                             ((float) server->player[ID].kills / fmaxf(1, (float) server->player[ID].deaths)),
                             server->player[ID].kills,
                             server->player[ID].deaths);
        } else {
            sendServerNotice(server, arguments.player, arguments.console, "ID not in range or player doesnt exist");
        }
    } else {
        if (arguments.console) {
            sendServerNotice(
            server, arguments.player, arguments.console, "You cannot use this command from console without argument");
            return;
        }
        sendServerNotice(
        server,
        arguments.player,
        arguments.console,
        "%s has kill to death ratio of: %f (Kills: %d, Deaths: %d)",
        server->player[arguments.player].name,
        ((float) server->player[arguments.player].kills / fmaxf(1, (float) server->player[arguments.player].deaths)),
        server->player[arguments.player].kills,
        server->player[arguments.player].deaths);
    }
}

static void resetCommand(void* serverP, CommandArguments arguments)
{
    Server* server = (Server*) serverP;
    (void) arguments;
    for (uint32 i = 0; i < server->protocol.maxPlayers; ++i) {
        if (server->player[i].state != STATE_DISCONNECTED) {
            server->player[i].state = STATE_STARTING_MAP;
        }
    }
    ServerReset(server);
}

static void sayCommand(void* serverP, CommandArguments arguments)
{
    Server* server = (Server*) serverP;
    if (arguments.argc == 2) {
        for (uint8 ID = 0; ID < server->protocol.maxPlayers; ++ID) {
            if (isPastJoinScreen(server, ID)) {
                sendServerNotice(server, ID, 0, arguments.argv[1]);
            }
        }
    } else {
        sendServerNotice(server, arguments.player, arguments.console, "Invalid message");
    }
}

static void serverCommand(void* serverP, CommandArguments arguments)
{
    Server* server = (Server*) serverP;
    sendServerNotice(
    server, arguments.player, arguments.console, "You are playing on SpadesX server. Version %s", VERSION);
}

static void toggleBuildCommand(void* serverP, CommandArguments arguments)
{
    Server* server = (Server*) serverP;
    uint8   ID;
    if (arguments.argc == 1) {
        if (server->globalAB == 1) {
            server->globalAB = 0;
            broadcastServerNotice(server, arguments.console, "Building has been disabled");
        } else if (server->globalAB == 0) {
            server->globalAB = 1;
            broadcastServerNotice(server, arguments.console, "Building has been enabled");
        }
    } else if (parsePlayer(arguments.argv[1], &ID, NULL)) {
        if (ID < server->protocol.maxPlayers && isPastJoinScreen(server, ID)) {
            if (server->player[ID].canBuild == 1) {
                server->player[ID].canBuild = 0;
                broadcastServerNotice(
                server, arguments.console, "Building has been disabled for %s", server->player[ID].name);
            } else if (server->player[ID].canBuild == 0) {
                server->player[ID].canBuild = 1;
                broadcastServerNotice(
                server, arguments.console, "Building has been enabled for %s", server->player[ID].name);
            }
        } else {
            sendServerNotice(server, arguments.player, arguments.console, "ID not in range or player doesnt exist");
        }
    }
}

static void toggleKillCommand(void* serverP, CommandArguments arguments)
{
    Server* server = (Server*) serverP;
    uint8   ID;
    if (arguments.argc == 1) {
        if (server->globalAK == 1) {
            server->globalAK = 0;
            broadcastServerNotice(server, arguments.console, "Killing has been disabled");
        } else if (server->globalAK == 0) {
            server->globalAK = 1;
            broadcastServerNotice(server, arguments.console, "Killing has been enabled");
        }
    } else if (parsePlayer(arguments.argv[1], &ID, NULL)) {
        if (ID < server->protocol.maxPlayers && isPastJoinScreen(server, ID)) {
            if (server->player[ID].allowKilling == 1) {
                server->player[ID].allowKilling = 0;
                broadcastServerNotice(
                server, arguments.console, "Killing has been disabled for %s", server->player[ID].name);
            } else if (server->player[ID].allowKilling == 0) {
                server->player[ID].allowKilling = 1;
                broadcastServerNotice(
                server, arguments.console, "Killing has been enabled for %s", server->player[ID].name);
            }
        } else {
            sendServerNotice(server, arguments.player, arguments.console, "ID not in range or player doesnt exist");
        }
    }
}

static void toggleTeamKillCommand(void* serverP, CommandArguments arguments)
{
    Server* server = (Server*) serverP;
    uint8   ID     = 33;
    if (arguments.argc == 2 && parsePlayer(arguments.argv[1], &ID, NULL)) {
        if (ID < server->protocol.maxPlayers && isPastJoinScreen(server, ID)) {
            if (server->player[ID].allowTeamKilling) {
                server->player[ID].allowTeamKilling = 0;
                broadcastServerNotice(
                server, arguments.console, "Team killing has been disabled for %s", server->player[ID].name);
            } else {
                server->player[ID].allowTeamKilling = 1;
                broadcastServerNotice(
                server, arguments.console, "Team killing has been enabled for %s", server->player[ID].name);
            }
        } else {
            sendServerNotice(server, arguments.player, arguments.console, "ID not in range or player doesnt exist");
        }
    } else {
        sendServerNotice(
        server, arguments.player, arguments.console, "You did not enter ID or entered incorrect argument");
    }
}

static void tpCommand(void* serverP, CommandArguments arguments)
{
    Server* server = (Server*) serverP;
    if (arguments.console) {
        LOG_INFO("You cannot use this command from console");
        return;
    }
    uint8 playerToBeTeleported = 33;
    uint8 playerToTeleportTo   = 33;
    if (arguments.argc == 3 && parsePlayer(arguments.argv[1], &playerToBeTeleported, NULL) &&
        parsePlayer(arguments.argv[2], &playerToTeleportTo, NULL))
    {
        if (vecfValidPos(server, server->player[playerToTeleportTo].movement.position)) {
            server->player[playerToBeTeleported].movement.position.x =
            server->player[playerToTeleportTo].movement.position.x - 0.5f;
            server->player[playerToBeTeleported].movement.position.y =
            server->player[playerToTeleportTo].movement.position.y - 0.5f;
            server->player[playerToBeTeleported].movement.position.z =
            server->player[playerToTeleportTo].movement.position.z - 2.36f;
            SendPositionPacket(server,
                               playerToBeTeleported,
                               server->player[playerToBeTeleported].movement.position.x,
                               server->player[playerToBeTeleported].movement.position.y,
                               server->player[playerToBeTeleported].movement.position.z);
        } else {
            sendServerNotice(
            server, arguments.player, arguments.console, "Player %hhu is at invalid position", playerToTeleportTo);
        }
    } else {
        sendServerNotice(
        server, arguments.player, arguments.console, "Incorrect amount of arguments or wrong argument type");
    }
}

static void tpcCommand(void* serverP, CommandArguments arguments)
{
    Server* server = (Server*) serverP;
    if (arguments.console) {
        LOG_INFO("You cannot use this command from console");
        return;
    }
    Vector3f pos = {0, 0, 0};
    if (arguments.argc == 4 && PARSE_VECTOR3F(arguments, 1, &pos)) {
        if (vecfValidPos(server, pos)) {
            server->player[arguments.player].movement.position.x = pos.x - 0.5f;
            server->player[arguments.player].movement.position.y = pos.y - 0.5f;
            server->player[arguments.player].movement.position.z = pos.z - 2.36f;
            SendPositionPacket(server,
                               arguments.player,
                               server->player[arguments.player].movement.position.x,
                               server->player[arguments.player].movement.position.y,
                               server->player[arguments.player].movement.position.z);
        } else {
            sendServerNotice(server, arguments.player, arguments.console, "Invalid position");
        }
    } else {
        sendServerNotice(
        server, arguments.player, arguments.console, "Incorrect amount of arguments or wrong argument type");
    }
}

static void unbanCommand(void* serverP, CommandArguments arguments)
{
    Server*  server = (Server*) serverP;
    IPStruct IP;
    uint8    unbanned = 0;
    if (arguments.argc == 2 && parseIP(arguments.argv[1], &IP, NULL)) {
        struct json_object* array;
        struct json_object* root = json_object_from_file("Bans.json");
        json_object_object_get_ex(root, "Bans", &array);
        int         count = json_object_array_length(array);
        const char* IPString;
        char        unbanIPString[19];
        formatIPToString(unbanIPString, IP);
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
            sendServerNotice(server, arguments.player, arguments.console, "IP %s unbanned", IPString);
        } else {
            sendServerNotice(
            server, arguments.player, arguments.console, "IP %s not found in banned IP list", unbanIPString);
        }
    } else {
        sendServerNotice(server, arguments.player, arguments.console, "Incorrect amount of arguments or invalid IP");
    }
}

static void unbanRangeCommand(void* serverP, CommandArguments arguments)
{
    Server*  server = (Server*) serverP;
    IPStruct startRange, endRange;
    char*    end;
    uint8    unbanned = 0;
    if (arguments.argc == 2 && parseIP(arguments.argv[1], &startRange, &end) && parseIP(end, &endRange, NULL)) {
        struct json_object* array;
        struct json_object* root = json_object_from_file("Bans.json");
        json_object_object_get_ex(root, "Bans", &array);
        int         count = json_object_array_length(array);
        const char* startIPString;
        const char* endIPString;
        char        unbanStartRangeString[19];
        char        unbanEndRangeString[19];
        formatIPToString(unbanStartRangeString, startRange);
        formatIPToString(unbanEndRangeString, endRange);
        for (int i = 0; i < count; ++i) {
            struct json_object* objectAtIndex = json_object_array_get_idx(array, i);
            READ_STR_FROM_JSON(objectAtIndex, startIPString, start_of_range, "start of range", "0.0.0.0", 0);
            READ_STR_FROM_JSON(objectAtIndex, endIPString, end_of_range, "end of range", "0.0.0.0", 0);
            if (strcmp(unbanStartRangeString, startIPString) == 0 && strcmp(unbanEndRangeString, endIPString) == 0) {
                json_object_array_del_idx(array, i, 1);
                json_object_to_file("Bans.json", root);
                unbanned = 1;
            }
        }
        if (unbanned) {
            sendServerNotice(
            server, arguments.player, arguments.console, "IP range %s-%s unbanned", startIPString, endIPString);
        } else {
            sendServerNotice(server,
                             arguments.player,
                             arguments.console,
                             "IP range %s-%s not found in banned IP ranges",
                             unbanStartRangeString,
                             unbanEndRangeString);
        }
    } else {
        sendServerNotice(server, arguments.player, arguments.console, "Incorrect amount of arguments or invalid IP");
    }
}

static void undobanCommand(void* serverP, CommandArguments arguments)
{
    Server*     server = (Server*) serverP;
    const char* IPString;
    const char* startIP;
    const char* endIP;
    if (arguments.argc == 1) {
        struct json_object* array;
        struct json_object* root = json_object_from_file("Bans.json");
        json_object_object_get_ex(root, "Bans", &array);
        int                 count         = json_object_array_length(array);
        struct json_object* objectAtIndex = json_object_array_get_idx(array, count - 1);
        READ_STR_FROM_JSON(objectAtIndex, IPString, IP, "IP", "0.0.0.0", 1);
        if (IPString[0] == '0') {
            READ_STR_FROM_JSON(objectAtIndex, startIP, start_of_range, "start of range", "0.0.0.0", 0);
            READ_STR_FROM_JSON(objectAtIndex, endIP, end_of_range, "end of range", "0.0.0.0", 0);
            json_object_array_del_idx(array, count - 1, 1);
            json_object_to_file("Bans.json", root);
            sendServerNotice(server, arguments.player, arguments.console, "IP range %s-%s unbanned", startIP, endIP);
        } else {
            json_object_array_del_idx(array, count - 1, 1);
            json_object_to_file("Bans.json", root);
            sendServerNotice(server, arguments.player, arguments.console, "IP %s unbanned", IPString);
        }
    } else {
        sendServerNotice(server, arguments.player, arguments.console, "Too many arguments given to command");
    }
}

static void upsCommand(void* serverP, CommandArguments arguments)
{
    Server* server = (Server*) serverP;
    if (arguments.console) {
        LOG_INFO("You cannot use this command from console");
        return;
    }
    if (arguments.argc != 2) {
        return;
    }
    float ups = 0;
    parseFloat(arguments.argv[1], &ups, NULL);
    if (ups >= 10 && ups <= 300) {
        server->player[arguments.player].ups = ups;
        sendServerNotice(server, arguments.player, arguments.console, "UPS changed to %.2f successfully", ups);
    } else {
        sendServerNotice(
        server, arguments.player, arguments.console, "Changing UPS failed. Please select value between 1 and 300");
    }
}

void createCommand(Server* server,
                   uint8   parseArguments,
                   void (*command)(void* serverP, CommandArguments arguments),
                   char   id[30],
                   char   commandDesc[1024],
                   uint32 permLevel)
{
    Command* structCommand        = malloc(sizeof(Command));
    structCommand->command        = command;
    structCommand->parseArguments = parseArguments;
    structCommand->PermLevel      = permLevel;
    strcpy(structCommand->commandDesc, commandDesc);
    strcpy(structCommand->id, id);
    HASH_ADD_STR(server->commandsMap, id, structCommand);
    LL_APPEND(server->commandsList, structCommand);
}

void populateCommands(Server* server)
{
    server->commandsList = NULL; // UTlist requires root to be initialized to NULL
    //  Add custom commands into this array definition
    CommandManager CommandArray[] = {
    {"/admin", 0, &adminCommand, 0, "Sends a message to all online admins."},
    {"/adminmute", 1, &adminMuteCommand, 30, "Mutes or unmutes player from /admin usage"},
    {"/ban", 0, &banCustomCommand, 30, "Puts specified IP into ban list"},
    {"/banrange", 0, &banRangeCommand, 30, "Bans specified IP range"},
    // We can have 2+ commands for same function even with different permissions and name
    {"/client", 1, &clinCommand, 0, "Shows players client info"},
    {"/clin", 1, &clinCommand, 0, "Shows players client info"},
    {"/dban", 0, &banCustomCommand, 30, "Bans specified player for a day"},
    {"/hban", 0, &banCustomCommand, 30, "Bans specified player for 6 hours"},
    {"/help", 0, &helpCommand, 0, "Shows commands and their description"},
    {"/intel", 0, &intelCommand, 0, "Shows info about intel"},
    {"/inv", 0, &invCommand, 30, "Makes you go invisible"},
    {"/kick", 1, &kickCommand, 30, "Kicks specified player from the server"},
    {"/kill", 1, &killCommand, 0, "Kills player who sent it or player specified in argument"},
    {"/login", 1, &loginCommand, 0, "Login command. First argument is a role. Second password"},
    {"/logout", 0, &logoutCommand, 31, "Logs out logged in player"},
    {"/master", 0, &masterCommand, 28, "Toggles master connection"},
    {"/mban", 0, &banCustomCommand, 30, "Bans specified player for a month"},
    {"/mute", 1, &muteCommand, 30, "Mutes or unmutes specified player"},
    {"/pban", 0, &banCustomCommand, 30, "Permanently bans a specified player"},
    {"/pm", 0, &pmCommand, 0, "Private message to specified player"},
    {"/ratio", 1, &ratioCommand, 0, "Shows yours or requested player ratio"},
    {"/reset", 0, &resetCommand, 24, "Resets server and loads next map"},
    {"/say", 0, &sayCommand, 30, "Send message to everyone as the server"},
    {"/server", 0, &serverCommand, 0, "Shows info about the server"},
    {"/tb", 1, &toggleBuildCommand, 30, "Toggles ability to build for everyone or specified player"},
    {"/tk", 1, &toggleKillCommand, 30, "Toggles ability to kill for everyone or specified player"},
    {"/tp", 1, &tpCommand, 24, "Teleports specified player to another specified player"},
    {"/tpc", 1, &tpcCommand, 24, "Teleports to specified cordinates"},
    {"/ttk", 1, &toggleTeamKillCommand, 30, "Toggles ability to team kill for everyone or specified player"},
    {"/unban", 0, &unbanCommand, 30, "Unbans specified IP"},
    {"/unbanrange", 0, &unbanRangeCommand, 30, "Unbans specified IP range"},
    {"/undoban", 0, &undobanCommand, 30, "Reverts the last ban"},
    {"/ups", 1, &upsCommand, 0, "Sets UPS of player to requested ammount. Range: 1-300"},
    {"/wban", 0, &banCustomCommand, 30, "Bans specified player for a week"}};
    for (unsigned long i = 0; i < sizeof(CommandArray) / sizeof(CommandManager); i++) {
        createCommand(server,
                      CommandArray[i].parseArguments,
                      CommandArray[i].command,
                      CommandArray[i].id,
                      CommandArray[i].commandDesc,
                      CommandArray[i].PermLevel);
    }
    LL_SORT(server->commandsList, commandCompare);
}

void freeCommands(Server* server)
{
    Command* currentCommand;
    Command* tmp;

    HASH_ITER(hh, server->commandsMap, currentCommand, tmp)
    {
        deleteCommand(server, currentCommand);
    }
}

void deleteCommand(Server* server, Command* command)
{
    HASH_DEL(server->commandsMap, command);
    LL_DELETE(server->commandsList, command);
    free(command);
}

uint8 parseArguments(Server* server, CommandArguments* arguments, char* message, uint8 commandLength)
{
    char* p = message + commandLength; // message beginning + command length
    while (*p != '\0' && arguments->argc < 32) {
        uint8 escaped = 0, quotesCount = 0, argumentLength;
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
                            sendServerNotice(server,
                                             arguments->player,
                                             arguments->console,
                                             "Failed to parse the command: found a stray \" symbol");
                            return 0;
                        }
                        char next = *(end + 1);
                        if (next != ' ' && next != '\t' && next != '\0') {
                            sendServerNotice(server,
                                             arguments->player,
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
            sendServerNotice(
            server, arguments->player, arguments->console, "Failed to parse the command: missing a \" symbol");
            return 0;
        }
    argparse_loop_exit:
        argumentLength = end - p;
        if (quotesCount == 2) {
            argumentLength--; // don't need that last quote mark
        }
        if (argumentLength) {
            char* argument           = malloc(argumentLength + 1); // Don't forget about the null terminator!
            argument[argumentLength] = '\0';
            memcpy(argument, p, argumentLength);
            arguments->argv[arguments->argc++] = argument;
        }
        p = end;
    }
    return 1;
}

void handleCommands(Server* server, uint8 player, char* message, uint8 console)
{
    Player srvPlayer;
    if (!console) {
        srvPlayer = server->player[player];
    }
    char* command = calloc(1000, sizeof(uint8));
    sscanf(message, "%s", command);
    uint8 commandLength = strlen(command);
    for (uint8 i = 1; i < commandLength; ++i) {
        message[i] = command[i] = tolower(command[i]);
    }

    Command* commandStruct;
    HASH_FIND_STR(server->commandsMap, command, commandStruct);
    if (commandStruct == NULL) {
        free(command);
        return;
    }

    CommandArguments arguments;
    arguments.player = player;
    if (!console) {
        arguments.srvPlayer = srvPlayer;
    }
    arguments.commandPermissions = commandStruct->PermLevel;
    arguments.console            = console;

    arguments.argv[0] = command;
    arguments.argc    = 1;

    uint8 messageLength;
    if (commandStruct->parseArguments) {
        if (!parseArguments(server, &arguments, message, commandLength)) {
            goto epic_parsing_fail;
        }
    } else if ((messageLength = strlen(message)) - commandLength > 1)
    { // if we have something other than the command itself
        char* argument = calloc(messageLength - commandLength, sizeof(message[0]));
        strcpy(argument, message + commandLength + 1);
        arguments.argv[arguments.argc++] = argument;
    }

    if (playerHasPermission(server, player, console, commandStruct->PermLevel) > 0 || commandStruct->PermLevel == 0) {
        commandStruct->command((void*) server, arguments);
    } else {
        sendServerNotice(server, player, console, "You do not have permissions to use this command");
    }
epic_parsing_fail:
    for (uint8 i = 1; i < arguments.argc; i++) { // Starting from 1 since we'll free the command anyway
        free(arguments.argv[i]);
    }
    free(command);
}
