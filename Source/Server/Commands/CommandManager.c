#include <Server/Commands/CommandManager.h>
#include <Server/Commands/Commands.h>
#include <Server/Master.h>
#include <Server/Packets/Packets.h>
#include <Server/ParseConvert.h>
#include <Server/Server.h>
#include <Server/Structs/ServerStruct.h>
#include <Util/Enums.h>
#include <Util/JSONHelpers.h>
#include <Util/Log.h>
#include <Util/Notice.h>
#include <Util/Types.h>
#include <Util/Uthash.h>
#include <Util/Utlist.h>
#include <Util/Alloc.h>
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

uint8_t player_has_permission(player_t* player, uint8_t console, uint32_t permission)
{
    if (console) {
        return permission;
    } else {
        return (permission & player->permissions);
    }
}

void cmd_generate_ban(server_t* server, command_args_t arguments, float time, ip_t ip, char* reason)
{
    char ipString[19];
    format_ip_to_str(ipString, ip);
    struct json_object* array;
    struct json_object* ban        = json_object_new_object();
    struct json_object* root       = json_object_from_file("Bans.json");
    char*               nameString = "Deuce";
    json_object_object_get_ex(root, "Bans", &array);

    uint8_t   banned = 0;
    player_t *connected_player, *tmp;
    HASH_ITER(hh, server->players, connected_player, tmp)
    {
        if (connected_player->state != STATE_DISCONNECTED && connected_player->ip.ip32 == ip.ip32) {
            if (banned == 0) {
                nameString = connected_player->name;
                banned     = 1; // Do not add multiples of the same IP. Its pointless.
            }
            enet_peer_disconnect(connected_player->peer, REASON_BANNED);
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
        send_server_notice(arguments.player, arguments.console, "IP %s has been permanently banned", ipString);
    } else {
        send_server_notice(arguments.player, arguments.console, "IP %s has been for %f minutes", ipString, time);
    }
}

void command_create(server_t* server,
                    uint8_t   parse_args,
                    void (*command)(void* p_server, command_args_t arguments),
                    char     id[30],
                    char     description[1024],
                    uint32_t permissions)
{
    command_t* cmd   = spadesx_malloc(sizeof(command_t));
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
    {"/admin", 0, &cmd_admin, 0, "Sends a message to all online admins."},
    {"/adminmute", 1, &cmd_admin_mute, 30, "Mutes or unmutes player from /admin usage"},
    {"/ban", 0, &cmd_ban_custom, 30, "Puts specified IP into ban list"},
    {"/banrange", 0, &cmd_ban_range, 30, "Bans specified IP range"},
    // We can have 2+ commands for same function even with different permissions and name
    {"/client", 1, &cmd_clin, 0, "Shows players client info"},
    {"/clin", 1, &cmd_clin, 0, "Shows players client info"},
    {"/dban", 0, &cmd_ban_custom, 30, "Bans specified player for a day"},
    {"/hban", 0, &cmd_ban_custom, 30, "Bans specified player for 6 hours"},
    {"/help", 0, &cmd_help, 0, "Shows commands and their description"},
    {"/intel", 0, &cmd_intel, 0, "Shows info about intel"},
    {"/inv", 0, &cmd_inv, 30, "Makes you go invisible"},
    {"/kick", 1, &cmd_kick, 30, "Kicks specified player from the server"},
    {"/kill", 1, &cmd_kill, 0, "Kills player who sent it or player specified in argument"},
    {"/login", 1, &cmd_login, 0, "Login command. First argument is a role. Second password"},
    {"/logout", 0, &cmd_logout, 31, "Logs out logged in player"},
    {"/master", 0, &cmd_master, 28, "Toggles master connection"},
    {"/mban", 0, &cmd_ban_custom, 30, "Bans specified player for a month"},
    {"/mute", 1, &cmd_mute, 30, "Mutes or unmutes specified player"},
    {"/pban", 0, &cmd_ban_custom, 30, "Permanently bans a specified player"},
    {"/pm", 0, &cmd_pm, 0, "Private message to specified player"},
    {"/ratio", 1, &cmd_ratio, 0, "Shows yours or requested player ratio"},
    {"/reset", 0, &cmd_reset, 24, "Resets server and loads next map"},
    {"/say", 0, &cmd_say, 30, "Send message to everyone as the server"},
    {"/server", 0, &cmd_server, 0, "Shows info about the server"},
    {"/tb", 1, &cmd_toggle_build, 30, "Toggles ability to build for everyone or specified player"},
    {"/tk", 1, &cmd_toggle_kill, 30, "Toggles ability to kill for everyone or specified player"},
    {"/tp", 1, &cmd_tp, 24, "Teleports specified player to another specified player"},
    {"/tpc", 1, &cmd_tpc, 24, "Teleports to specified cordinates"},
    {"/ttk", 1, &cmd_toggle_team_kill, 30, "Toggles ability to team kill for everyone or specified player"},
    {"/unban", 0, &cmd_unban, 30, "Unbans specified IP"},
    {"/unbanrange", 0, &cmd_unban_range, 30, "Unbans specified IP range"},
    {"/undoban", 0, &cmd_undo_ban, 30, "Reverts the last ban"},
    {"/ups", 1, &cmd_ups, 0, "Sets UPS of player to requested ammount. Range: 1-300"},
    {"/wban", 0, &cmd_ban_custom, 30, "Bans specified player for a week"},
    {"/shutdown", 0, &cmd_shutdown, 16, "Shutdown the server"}};
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

void free_all_commands(server_t* server)
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

static uint8_t parse_arguments(command_args_t* arguments, char* message, uint8_t commandLength)
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
                            send_server_notice(arguments->player,
                                               arguments->console,
                                               "Failed to parse the command: found a stray \" symbol");
                            return 0;
                        }
                        char next = *(end + 1);
                        if (next != ' ' && next != '\t' && next != '\0') {
                            send_server_notice(arguments->player,
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
            arguments->player, arguments->console, "Failed to parse the command: missing a \" symbol");
            return 0;
        }
    argparse_loop_exit:
        argument_length = end - p;
        if (quotesCount == 2) {
            argument_length--; // don't need that last quote mark
        }
        if (argument_length) {
            char* argument            = spadesx_malloc(argument_length + 1);
            // Don't forget about the null terminator!
            argument[argument_length] = '\0';
            memcpy(argument, p, argument_length);
            arguments->argv[arguments->argc++] = argument;
        }
        p = end;
    }
    return 1;
}

void command_handle(server_t* server, player_t* player, char* message, uint8_t console)
{
    if (strlen(message) > 1000)
    {
        send_server_notice(player, console, "This command is longer then the 1000 character limit. Thus it has been ignored");
        return;
    }
    char* command = spadesx_calloc(1000, sizeof(char));
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
    arguments.player      = player;
    arguments.permissions = cmd->permissions;
    arguments.console     = console;
    arguments.argv[0]     = command;
    arguments.argc        = 1;
    arguments.server = server;

    uint8_t message_length;
    if (cmd->parse_args) {
        if (!parse_arguments(&arguments, message, command_length)) {
            goto epic_parsing_fail;
        }
    } else if ((message_length = strlen(message)) - command_length > 1)
    { // if we have something other than the command itself
        char* argument = spadesx_calloc(message_length - command_length, sizeof(message[0]));
        strcpy(argument, message + command_length + 1);
        arguments.argv[arguments.argc++] = argument;
    }

    if (player_has_permission(player, console, cmd->permissions) > 0 || cmd->permissions == 0) {
        cmd->execute((void*) server, arguments);
    } else {
        send_server_notice(player, console, "You do not have permissions to use this command");
    }
epic_parsing_fail:
    for (uint8_t i = 1; i < arguments.argc; i++) { // Starting from 1 since we'll free the command anyway
        free(arguments.argv[i]);
    }
    free(command);
}
