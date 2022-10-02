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
