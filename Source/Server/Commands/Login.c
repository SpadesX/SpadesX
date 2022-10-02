#include <Server/Server.h>
#include <Util/Log.h>
#include <Util/Notice.h>
#include <ctype.h>

void cmd_login(void* p_server, command_args_t arguments)
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

void cmd_logout(void* p_server, command_args_t arguments)
{
    server_t* server = (server_t*) p_server;
    if (arguments.console) {
        LOG_INFO("You cannot use this command from console");
        return;
    }
    server->player[arguments.player_id].permissions = 0;
    send_server_notice(server, arguments.player_id, arguments.console, "You logged out");
}
