#include <Server/Server.h>
#include <Server/Staff.h>
#include <Util/Notice.h>

void cmd_admin(void* p_server, command_args_t arguments)
{
    server_t* server = (server_t*) p_server;
    if (arguments.argc > 1) {
        if (arguments.console == 0 && server->player[arguments.player_id].admin_muted == 1) {
            send_server_notice(
            server, arguments.player_id, arguments.console, "You are not allowed to use this command (Admin muted)");
            return;
        }
        if (arguments.console) {
            send_message_to_staff(server, "Staff from %s: %s", "Console", arguments.argv[1]);
        } else {
            send_message_to_staff(
            server, "Staff from %s: %s", server->player[arguments.player_id].name, arguments.argv[1]);
        }
        send_server_notice(server, arguments.player_id, arguments.console, "Message sent to all staff members online");
    } else {
        send_server_notice(server, arguments.player_id, arguments.console, "Invalid message");
    }
}
