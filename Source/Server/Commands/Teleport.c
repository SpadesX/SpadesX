#include <Server/Packets/Packets.h>
#include <Server/ParseConvert.h>
#include <Server/Server.h>
#include <Util/Checks/PositionChecks.h>
#include <Util/Log.h>
#include <Util/Notice.h>

void cmd_tp(void* p_server, command_args_t arguments)
{
    server_t* server = (server_t*) p_server;
    if (arguments.console) {
        LOG_INFO("You cannot use this command from console");
        return;
    }
    uint8_t to_be_teleported = 33;
    uint8_t to_teleport_to   = 33;
    if (arguments.argc == 3 && parse_player(arguments.argv[1], &to_be_teleported, NULL) &&
        parse_player(arguments.argv[2], &to_teleport_to, NULL))
    {
        player_t *player_to_be_teleported, *player_to_teleport_to;
        HASH_FIND(hh, server->players, &to_teleport_to, sizeof(to_teleport_to), player_to_teleport_to);
        HASH_FIND(hh, server->players, &to_be_teleported, sizeof(to_be_teleported), player_to_be_teleported);
        if (player_to_teleport_to == NULL || player_to_be_teleported == NULL) {
            send_server_notice(arguments.player, arguments.console, "ID not in range or player doesnt exist");
            return;
        }
        if (valid_pos_v3f(server, player_to_teleport_to->movement.position)) {
            movement_t* from = &player_to_be_teleported->movement;
            movement_t* to   = &player_to_teleport_to->movement;
            from->position.x = to->position.x - 0.5f;
            from->position.y = to->position.y - 0.5f;
            from->position.z = to->position.z - 2.36f;
            send_position_packet(server, player_to_be_teleported, from->position.x, from->position.y, from->position.z);
        } else {
            send_server_notice(
            arguments.player, arguments.console, "Player %hhu is at invalid position", player_to_teleport_to);
        }
    } else {
        send_server_notice(arguments.player, arguments.console, "Incorrect amount of arguments or wrong argument type");
    }
}

void cmd_tpc(void* p_server, command_args_t arguments)
{
    server_t* server = (server_t*) p_server;
    if (arguments.console) {
        LOG_INFO("You cannot use this command from console");
        return;
    }
    vector3f_t pos = {0, 0, 0};
    if (arguments.argc == 4 && PARSE_VECTOR3F(arguments, 1, &pos)) {
        if (valid_pos_v3f(server, pos)) {
            arguments.player->movement.position.x = pos.x - 0.5f;
            arguments.player->movement.position.y = pos.y - 0.5f;
            arguments.player->movement.position.z = pos.z - 2.36f;
            send_position_packet(server,
                                 arguments.player,
                                 arguments.player->movement.position.x,
                                 arguments.player->movement.position.y,
                                 arguments.player->movement.position.z);
        } else {
            send_server_notice(arguments.player, arguments.console, "Invalid position");
        }
    } else {
        send_server_notice(arguments.player, arguments.console, "Incorrect amount of arguments or wrong argument type");
    }
}
