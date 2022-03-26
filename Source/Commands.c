#include "Commands.h"

#include "Master.h"
#include "Packets.h"
#include "Protocol.h"
#include "Structs.h"
#include "Types.h"
#include "Uthash.h"
#include "Utlist.h"

#include <Enums.h>
#include <ctype.h>
#include <enet/enet.h>
#include <errno.h>
#include <inttypes.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

static uint32 commandCompare(Command* first, Command* second)
{
    return strcmp(first->id, second->id);
}

static void adminCommand(void* serverP, CommandArguments arguments)
{
    Server* server = (Server*) serverP;
    if (arguments.argc > 1) {
        if (server->player[arguments.player].adminMuted == 1) {
            sendServerNotice(server, arguments.player, "You are not allowed to use this command (Admin muted)");
            return;
        }
        sendMessageToStaff(server, "Staff from %s: %s", server->player[arguments.player].name, arguments.argv[1]);
        sendServerNotice(server, arguments.player, "Message sent to all staff members online");
    } else {
        sendServerNotice(server, arguments.player, "Invalid message");
    }
}

static void adminMuteCommand(void* serverP, CommandArguments arguments)
{
    Server* server = (Server*) serverP;
    int     ID     = 33;
    if (arguments.argc == 2 && parsePlayer(arguments.argv[1], &ID, NULL)) {
        if (ID >= 0 && ID < 31 && isPastJoinScreen(server, ID)) {
            if (server->player[ID].adminMuted) {
                server->player[ID].adminMuted = 0;
                sendServerNotice(server, arguments.player, "%s has been admin unmuted", server->player[ID].name);
            } else {
                server->player[ID].adminMuted = 1;
                sendServerNotice(server, arguments.player, "%s has been admin muted", server->player[ID].name);
            }
        } else {
            sendServerNotice(server, arguments.player, "ID not in range or player doesnt exist");
        }
    } else {
        sendServerNotice(server, arguments.player, "You did not enter ID or entered incorrect argument");
    }
}

static void banIPCommand(void* serverP, CommandArguments arguments)
{
    Server* server = (Server*) serverP;
    IPUnion ip;
    if (arguments.argc == 2) {
        if (parseIP(arguments.argv[1], &ip)) {
            char ipString[16];
            FORMAT_IP(ipString, ip); // Reformatting the IP to avoid stuff like 001.02.3.4
            FILE* fp;
            fp = fopen("BanList.txt", "a");
            if (fp == NULL) {
                sendServerNotice(server, arguments.player, "IP could not be banned. File failed to open");
                return;
            }
            uint8 banned = 0;
            for (uint8 ID = 0; ID < server->protocol.maxPlayers; ++ID) {
                if (server->player[ID].state != STATE_DISCONNECTED && server->player[ID].ipUnion.ip32 == ip.ip32) {
                    if (banned == 0) {
                        fprintf(fp, "%s, %s,\n", ipString, server->player[ID].name);
                        fclose(fp);
                        banned = 1; // Do not add multiples of the same IP. Its pointless.
                    }
                    enet_peer_disconnect(server->player[ID].peer, REASON_BANNED);
                }
            }
            sendServerNotice(server, arguments.player, "IP %s has been permanently banned", ipString);
        } else {
            sendServerNotice(server, arguments.player, "Invalid IP format");
        }
    } else {
        sendServerNotice(server, arguments.player, "You did not enter IP or entered incorrect argument");
    }
}

static void clinCommand(void* serverP, CommandArguments arguments)
{
    Server* server = (Server*) serverP;
    int     ID     = 33;
    if (arguments.argc == 2 && parsePlayer(arguments.argv[1], &ID, NULL)) {
        if (ID >= 0 && ID < 31 && isPastJoinScreen(server, ID)) {
            char client[15];
            if (server->player[arguments.player].client == 'o') {
                snprintf(client, 12, "OpenSpades");
            } else if (server->player[arguments.player].client == 'B') {
                snprintf(client, 13, "BetterSpades");
            } else {
                snprintf(client, 7, "Voxlap");
            }
            sendServerNotice(server,
                             arguments.player,
                             "Player %s is running %s version %d.%d.%d on %s",
                             server->player[ID].name,
                             client,
                             server->player[ID].version_major,
                             server->player[ID].version_minor,
                             server->player[ID].version_revision,
                             server->player[ID].os_info);
        } else {
            sendServerNotice(server, arguments.player, "Invalid ID. Player doesnt exist");
        }
    } else {
        sendServerNotice(server, arguments.player, "No ID given");
    }
}

static void helpCommand(void* serverP, CommandArguments arguments)
{
    Server*  server = (Server*) serverP;
    Command* command;
    LL_FOREACH(server->commandsList, command)
    {
        if (playerHasPermission(server, arguments.player, command->PermLevel) || command->PermLevel == 0) {
            sendServerNotice(
            server, arguments.player, "[Command: %s, Description: %s]", command->id, command->commandDesc);
        }
    }
    sendServerNotice(server, arguments.player, "Commands available to you:");
}

static void intelCommand(void* serverP, CommandArguments arguments)
{
    Server* server          = (Server*) serverP;
    uint8   sentAtLeastOnce = 0;
    for (uint8 playerID = 0; playerID < server->protocol.maxPlayers; ++playerID) {
        if (server->player[playerID].state != STATE_DISCONNECTED && server->player[playerID].hasIntel) {
            sendServerNotice(server, arguments.player, "Player #%d has intel", playerID);
            sentAtLeastOnce = 1;
        }
    }
    if (sentAtLeastOnce == 0) {
        if (server->protocol.gameMode.intelHeld[0]) {
            sendServerNotice(server, arguments.player, "Intel is not being held but intel of team 0 thinks it is");
        } else if (server->protocol.gameMode.intelHeld[1]) {
            sendServerNotice(server, arguments.player, "Intel is not being held but intel of team 1 thinks it is");
        }
        sendServerNotice(server, arguments.player, "Intel is not being held");
    }
}

static void invCommand(void* serverP, CommandArguments arguments)
{
    Server* server = (Server*) serverP;
    if (server->player[arguments.player].isInvisible == 1) {
        server->player[arguments.player].isInvisible  = 0;
        server->player[arguments.player].allowKilling = 1;
        for (uint8 i = 0; i < server->protocol.maxPlayers; ++i) {
            if (isPastJoinScreen(server, i) && i != arguments.player) {
                SendRespawnState(server, i, arguments.player);
            }
        }
        sendServerNotice(server, arguments.player, "You are no longer invisible");
    } else if (server->player[arguments.player].isInvisible == 0) {
        server->player[arguments.player].isInvisible  = 1;
        server->player[arguments.player].allowKilling = 0;
        sendKillPacket(server, arguments.player, arguments.player, 0, 0, 1);
        sendServerNotice(server, arguments.player, "You are now invisible");
    }
}

static void kickCommand(void* serverP, CommandArguments arguments)
{
    Server* server = (Server*) serverP;
    int     ID     = 33;
    if (arguments.argc == 2 && parsePlayer(arguments.argv[1], &ID, NULL)) {
        if (ID >= 0 && ID < 31 && isPastJoinScreen(server, ID)) {
            enet_peer_disconnect(server->player[ID].peer, REASON_KICKED);
            broadcastServerNotice(server, "Player %s has been kicked", server->player[ID].name);
        } else {
            sendServerNotice(server, arguments.player, "ID not in range or player doesnt exist");
        }
    } else {
        sendServerNotice(server, arguments.player, "You did not enter ID or entered incorrect argument");
    }
}

static void killCommand(void* serverP, CommandArguments arguments)
{
    Server* server = (Server*) serverP;
    if (arguments.argc == 1) {
        sendKillPacket(server, arguments.player, arguments.player, 0, 5, 0);
    } else {
        if (!playerHasPermission(server, arguments.player, 30)) {
            sendServerNotice(server, arguments.player, "You have no permission to use this command.");
            return;
        }

        if (strcmp(arguments.argv[1], "all") == 0) { // KILL THEM ALL!!!! >:D
            int count = 0;
            for (int i = 0; i < server->protocol.maxPlayers; ++i) {
                if (isPastJoinScreen(server, i) && server->player[i].HP > 0 && server->player[i].team != TEAM_SPECTATOR) {
                    sendKillPacket(server, i, i, 0, 5, 0);
                    count++;
                }
            }
            sendServerNotice(server, arguments.player, "Killed %i players.", count);
            return;
        }

        int id = 0;
        for (uint32 i = 1; i < arguments.argc; i++) {
            if (!parsePlayer(arguments.argv[i], &id, NULL)) {
                sendServerNotice(server, arguments.player, "Invalid player \"%s\"!", arguments.argv[i]);
                return;
            }
            sendKillPacket(server, id, id, 0, 5, 0);
            sendServerNotice(server, arguments.player, "Killing player #%i...", id);
        }
    }
}

static void loginCommand(void* serverP, CommandArguments arguments)
{
    Server* server = (Server*) serverP;
    if (server->player[arguments.player].permLevel == 0) {
        if (arguments.argc == 3) {
            char *level = arguments.argv[1];
            char *password = arguments.argv[2];
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
                                         "You logged in as %s",
                                         server->player[arguments.player].roleList[i].accessLevel);
                        return;
                    } else {
                        sendServerNotice(server, arguments.player, "Wrong password");
                        return;
                    }
                } else {
                    failed++;
                }
            }
            if (failed >= sizeof(server->player[arguments.player].roleList) / sizeof(PermLevel)) {
                sendServerNotice(server, arguments.player, "Invalid role");
            }
        } else {
            sendServerNotice(server, arguments.player, "Incorrect number of arguments to login");
        }
    } else {
        sendServerNotice(server, arguments.player, "You are already logged in");
    }
}

static void logoutCommand(void* serverP, CommandArguments arguments)
{
    Server* server                             = (Server*) serverP;
    server->player[arguments.player].permLevel = 0;
    sendServerNotice(server, arguments.player, "You logged out");
}

static void masterCommand(void* serverP, CommandArguments arguments)
{
    Server* server = (Server*) serverP;
    if (server->master.enableMasterConnection == 1) {
        server->master.enableMasterConnection = 0;
        enet_host_destroy(server->master.client);
        sendServerNotice(server, arguments.player, "Disabling master connection");
        return;
    }
    server->master.enableMasterConnection = 1;
    ConnectMaster(server, server->port);
    sendServerNotice(server, arguments.player, "Enabling master connection");
}

static void muteCommand(void* serverP, CommandArguments arguments)
{
    Server* server = (Server*) serverP;
    int     ID     = 33;
    if (arguments.argc == 2 && parsePlayer(arguments.argv[1], &ID, NULL)) {
        if (ID >= 0 && ID < 31 && isPastJoinScreen(server, ID)) {
            if (server->player[ID].muted) {
                server->player[ID].muted = 0;
                broadcastServerNotice(server, "%s has been unmuted", server->player[ID].name);
            } else {
                server->player[ID].muted = 1;
                broadcastServerNotice(server, "%s has been muted", server->player[ID].name);
            }
        } else {
            sendServerNotice(server, arguments.player, "ID not in range or player doesnt exist");
        }
    } else {
        sendServerNotice(server, arguments.player, "You did not enter ID or entered incorrect argument");
    }
}

static void pbanCommand(void* serverP, CommandArguments arguments)
{
    Server* server = (Server*) serverP;
    int     ID     = 33;
    if (arguments.argc == 2 && parsePlayer(arguments.argv[1], &ID, NULL)) {
        if (ID >= 0 && ID < 31 && isPastJoinScreen(server, ID)) {
            FILE* fp;
            fp = fopen("BanList.txt", "a");
            if (fp == NULL) {
                sendServerNotice(server, arguments.player, "Player could not be banned. File failed to open");
                return;
            }
            fprintf(fp,
                    "%hhu.%hhu.%hhu.%hhu, %s,\n",
                    server->player[ID].ipUnion.ip[0],
                    server->player[ID].ipUnion.ip[1],
                    server->player[ID].ipUnion.ip[2],
                    server->player[ID].ipUnion.ip[3],
                    server->player[ID].name);
            fclose(fp);
            enet_peer_disconnect(server->player[ID].peer, REASON_BANNED);
            broadcastServerNotice(server, "%s has been permanently banned", server->player[ID].name);
        } else {
            sendServerNotice(server, arguments.player, "ID not in range or player doesnt exist");
        }
    } else {
        sendServerNotice(server, arguments.player, "You did not enter ID or entered incorrect argument");
    }
}

static void pmCommand(void* serverP, CommandArguments arguments)
{
    Server* server = (Server*) serverP;
    char*   PM;
    int     ID = 33;
    if (arguments.argc == 2 && parsePlayer(arguments.argv[1], &ID, &PM) && strlen(++PM) > 0) {
        if (ID >= 0 && ID < 31 && isPastJoinScreen(server, ID)) {
            sendServerNotice(server, ID, "PM from %s: %s", server->player[arguments.player].name, PM);
            sendServerNotice(server, arguments.player, "PM sent to %s", server->player[ID].name);
        } else {
            sendServerNotice(server, arguments.player, "Invalid ID. Player doesnt exist");
        }
    } else {
        sendServerNotice(server, arguments.player, "No ID or invalid message");
    }
}

static void ratioCommand(void* serverP, CommandArguments arguments)
{
    Server* server = (Server*) serverP;
    int     ID     = 33;
    if (arguments.argc == 2 && parsePlayer(arguments.argv[1], &ID, NULL)) {
        if (ID >= 0 && ID < 31 && isPastJoinScreen(server, ID)) {
            sendServerNotice(server,
                             arguments.player,
                             "%s has kill to death ratio of: %f (Kills: %d, Deaths: %d)",
                             server->player[ID].name,
                             ((float) server->player[ID].kills / fmaxf(1, (float) server->player[ID].deaths)),
                             server->player[ID].kills,
                             server->player[ID].deaths);
        } else {
            sendServerNotice(server, arguments.player, "ID not in range or player doesnt exist");
        }
    } else {
        sendServerNotice(
        server,
        arguments.player,
        "%s has kill to death ratio of: %f (Kills: %d, Deaths: %d)",
        server->player[arguments.player].name,
        ((float) server->player[arguments.player].kills / fmaxf(1, (float) server->player[arguments.player].deaths)),
        server->player[arguments.player].kills,
        server->player[arguments.player].deaths);
    }
}

static void sayCommand(void* serverP, CommandArguments arguments)
{
    Server* server = (Server*) serverP;
    if (arguments.argc == 2) {
        for (uint8 ID = 0; ID < server->protocol.maxPlayers; ++ID) {
            if (isPastJoinScreen(server, ID)) {
                sendServerNotice(server, ID, arguments.argv[1]);
            }
        }
    } else {
        sendServerNotice(server, arguments.player, "Invalid message");
    }
}

static void serverCommand(void* serverP, CommandArguments arguments)
{
    Server* server = (Server*) serverP;
    sendServerNotice(server, arguments.player, "You are playing on SpadesX server. Version %s", VERSION);
}

static void toggleBuildCommand(void* serverP, CommandArguments arguments)
{
    Server* server = (Server*) serverP;
    int     ID;
    if (arguments.argc == 1) {
        if (server->globalAB == 1) {
            server->globalAB = 0;
            broadcastServerNotice(server, "Building has been disabled");
        } else if (server->globalAB == 0) {
            server->globalAB = 1;
            broadcastServerNotice(server, "Building has been enabled");
        }
    } else if (parsePlayer(arguments.argv[1], &ID, NULL)) {
        if (ID >= 0 && ID < 31 && isPastJoinScreen(server, ID)) {
            if (server->player[ID].canBuild == 1) {
                server->player[ID].canBuild = 0;
                broadcastServerNotice(server, "Building has been disabled for %s", server->player[ID].name);
            } else if (server->player[ID].canBuild == 0) {
                server->player[ID].canBuild = 1;
                broadcastServerNotice(server, "Building has been enabled for %s", server->player[ID].name);
            }
        } else {
            sendServerNotice(server, arguments.player, "ID not in range or player doesnt exist");
        }
    }
}

static void toggleKillCommand(void* serverP, CommandArguments arguments)
{
    Server* server = (Server*) serverP;
    int     ID;
    if (arguments.argc == 1) {
        if (server->globalAK == 1) {
            server->globalAK = 0;
            broadcastServerNotice(server, "Killing has been disabled");
        } else if (server->globalAK == 0) {
            server->globalAK = 1;
            broadcastServerNotice(server, "Killing has been enabled");
        }
    } else if (parsePlayer(arguments.argv[1], &ID, NULL)) {
        if (ID >= 0 && ID < 31 && isPastJoinScreen(server, ID)) {
            if (server->player[ID].allowKilling == 1) {
                server->player[ID].allowKilling = 0;
                broadcastServerNotice(server, "Killing has been disabled for %s", server->player[ID].name);
            } else if (server->player[ID].allowKilling == 0) {
                server->player[ID].allowKilling = 1;
                broadcastServerNotice(server, "Killing has been enabled for %s", server->player[ID].name);
            }
        } else {
            sendServerNotice(server, arguments.player, "ID not in range or player doesnt exist");
        }
    }
}

static void toggleTeamKillCommand(void* serverP, CommandArguments arguments)
{
    Server* server = (Server*) serverP;
    int     ID     = 33;
    if (arguments.argc == 2 && parsePlayer(arguments.argv[1], &ID, NULL)) {
        if (ID >= 0 && ID < 31 && isPastJoinScreen(server, ID)) {
            if (server->player[ID].allowTeamKilling) {
                server->player[ID].allowTeamKilling = 0;
                broadcastServerNotice(server, "Team killing has been disabled for %s", server->player[ID].name);
            } else {
                server->player[ID].allowTeamKilling = 1;
                broadcastServerNotice(server, "Team killing has been enabled for %s", server->player[ID].name);
            }
        } else {
            sendServerNotice(server, arguments.player, "ID not in range or player doesnt exist");
        }
    } else {
        sendServerNotice(server, arguments.player, "You did not enter ID or entered incorrect argument");
    }
}

static void tpcCommand(void* serverP, CommandArguments arguments)
{
    Server*  server = (Server*) serverP;
    Vector3f pos    = {0, 0, 0};
    if (arguments.argc == 4 && PARSE_VECTOR3F(arguments, 1, &pos)) {
        if (vecfValidPos(pos)) {
            server->player[arguments.player].movement.position.x = pos.x - 0.5f;
            server->player[arguments.player].movement.position.y = pos.y - 0.5f;
            server->player[arguments.player].movement.position.z = pos.z - 2.36f;
            SendPositionPacket(server,
                               arguments.player,
                               server->player[arguments.player].movement.position.x,
                               server->player[arguments.player].movement.position.y,
                               server->player[arguments.player].movement.position.z);
        } else {
            sendServerNotice(server, arguments.player, "Invalid position");
        }
    } else {
        sendServerNotice(server, arguments.player, "Incorrect amount of arguments or wrong argument type");
    }
}

static void upsCommand(void* serverP, CommandArguments arguments)
{
    if (arguments.argc != 2) {
        return;
    }

    Server* server = (Server*) serverP;
    float   ups    = 0;
    parseFloat(arguments.argv[1], &ups, NULL);
    if (ups >= 1 && ups <= 300) {
        server->player[arguments.player].ups = ups;
        sendServerNotice(server, arguments.player, "UPS changed to %.2f successfully", ups);
    } else {
        sendServerNotice(server, arguments.player, "Changing UPS failed. Please select value between 1 and 300");
    }
}

void createCommand(Server* server,
                   uint8 parseArguments,
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
    {"/banip", 1, &banIPCommand, 30, "Puts specified IP into ban list"},
    // We can have 2+ commands for same function even with different permissions and name
    {"/client", 1, &clinCommand, 0, "Shows players client info"},
    {"/clin", 1, &clinCommand, 0, "Shows players client info"},
    {"/help", 0, &helpCommand, 0, "Shows commands and their description"},
    {"/intel", 0, &intelCommand, 0, "Shows info about intel"},
    {"/inv", 0, &invCommand, 30, "Makes you go invisible"},
    {"/kick", 1, &kickCommand, 30, "Kicks specified player from the server"},
    {"/kill", 1, &killCommand, 0, "Kills player who sent it or player specified in argument"},
    {"/login", 1, &loginCommand, 0, "Login command. First argument is a role. Second password"},
    {"/logout", 0, &logoutCommand, 31, "Logs out logged in player"},
    {"/master", 0, &masterCommand, 28, "Toggles master connection"},
    {"/mute", 1, &muteCommand, 30, "Mutes or unmutes specified player"},
    {"/pban", 1, &pbanCommand, 30, "Permanently bans a specified player"},
    {"/pm", 0, &pmCommand, 0, "Private message to specified player"},
    {"/ratio", 1, &ratioCommand, 0, "Shows yours or requested player ratio"},
    {"/say", 0, &sayCommand, 30, "Send message to everyone as the server"},
    {"/server", 0, &serverCommand, 0, "Shows info about the server"},
    {"/tb", 1, &toggleBuildCommand, 30, "Toggles ability to build for everyone or specified player"},
    {"/tk", 1, &toggleKillCommand, 30, "Toggles ability to kill for everyone or specified player"},
    {"/tpc", 1, &tpcCommand, 24, "Teleports to specified cordinates"},
    {"/ttk", 1, &toggleTeamKillCommand, 30, "Toggles ability to team kill for everyone or specified player"},
    {"/ups", 1, &upsCommand, 0, "Sets UPS of player to requested ammount. Range: 1-300"}
    };
    for (unsigned long i = 0; i < sizeof(CommandArray) / sizeof(CommandManager); i++) {
        createCommand(
        server, CommandArray[i].parseArguments, CommandArray[i].command, CommandArray[i].id, CommandArray[i].commandDesc, CommandArray[i].PermLevel);
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
    char* p = message + commandLength + 1; // message beginning + command length + one space symbol (pointer math ooOOooOOOOoOO)
    while (*p != '\0' && arguments->argc < 32) {
        uint8 escaped = 0, quotesCount = 0, argumentLength;
        while (*p == ' ' || *p == '\t') p++; // rewinding
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
                        sendServerNotice(server, arguments->player, "Failed to parse the command: found a stray \" symbol");
                        return 0;
                    }
                    char next = *(end + 1);
                    if (next != ' ' && next != '\t' && next != '\0') {
                        sendServerNotice(server, arguments->player, "Failed to parse the command: found more symbols after the \" symbol");
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
            sendServerNotice(server, arguments->player, "Failed to parse the command: missing a \" symbol");
            return 0;
        }
        argparse_loop_exit:
        argumentLength = end - p;
        if (quotesCount == 2) {
            argumentLength--; // don't need that last quote mark
        }
        if (argumentLength) {
            char* argument = malloc(argumentLength + 1); // Don't forget about the null terminator!
            argument[argumentLength] = '\0';
            memcpy(argument, p, argumentLength);
            arguments->argv[arguments->argc++] = argument;
        }
        p = end;
    }
    return 1;
}

void handleCommands(Server* server, uint8 player, char* message)
{
    Player srvPlayer = server->player[player];
    char*  command   = calloc(1000, sizeof(uint8));
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
    arguments.player             = player;
    arguments.srvPlayer          = srvPlayer;
    arguments.commandPermissions = commandStruct->PermLevel;

    arguments.argv[0] = command;
    arguments.argc = 1;

    uint8 messageLength;
    if (commandStruct->parseArguments) {
        if (!parseArguments(server, &arguments, message, commandLength)) {
            goto epic_parsing_fail;
        }
    } else if ((messageLength = strlen(message)) - commandLength > 1) { // if we have something other than the command itself
        char* argument = calloc(messageLength - commandLength, sizeof(message[0]));
        strcpy(argument, message + commandLength + 1);
        arguments.argv[arguments.argc++] = argument;
        LOG_DEBUG("%s", argument);
    }

    if (playerHasPermission(server, player, commandStruct->PermLevel) > 0 || commandStruct->PermLevel == 0) {
        commandStruct->command((void*) server, arguments);
    } else {
        sendServerNotice(server, player, "You do not have permissions to use this command");
    }
    epic_parsing_fail:
    for (uint8 i = 1; i < arguments.argc; i++) { // Starting from 1 since we'll free the command anyway
        free(arguments.argv[i]);
    }
    free(command);
}

uint8 parsePlayer(char* s, int* id, char** end)
{
    uint8 sLength;
    char* _end;
    if (s[0] != '#' || (sLength = strlen(s)) < 2) { // TODO: look up a player by their nickname if the argument doesn't start with #
        return 0;
    }

    *id = strtoimax(s + 1, &_end, 10);
    if (!_end || _end == s) {
        return 0;
    }

    if (end) {
        *end = _end;
    }
    return 1;
}

uint8 parseByte(char* s, uint8* byte, char** end)
{
    char* _end;
    int parsed = strtoimax(s, &_end, 10);
    if (!_end || _end == s || parsed < 0 || parsed > 255) {
        return 0;
    }

    *byte = parsed & 0xFF;
    if (end) {
        *end = _end;
    }
    return 1;
}

uint8 parseFloat(char* s, float* value, char** end)
{
    char* _end;
    double parsed = strtod(s, &_end);
    if (!_end || _end == s) {
        return 0;
    }

    *value = (float) parsed;
    if (end) {
        *end = _end;
    }
    return 1;
}

uint8 parseIP(char* s, IPUnion* ip)
{
    if (!parseByte(s, &ip->ip[0], &s)) {
        return 0;
    }

    for (int i = 1; i < 4; i++) {
        if (*s++ != '.' || !parseByte(s, &ip->ip[i], &s)) {
             return 0;
        }
    }

    if (*s != '\0') { // The pointer should point at the end of string in case it has no more characters. Fail otherwise.
        return 0;
    }

    return 1;
}
