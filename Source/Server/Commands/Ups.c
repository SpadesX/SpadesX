#include <Server/ParseConvert.h>
#include <Server/Server.h>
#include <Util/Log.h>
#include <Util/Notice.h>

void cmd_ups(void* p_server, command_args_t arguments)
{
    (void)p_server;
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
        arguments.player->ups = ups;
        send_server_notice(arguments.player, arguments.console, "UPS changed to %.2f successfully", ups);
    } else {
        send_server_notice(arguments.player, arguments.console, "Changing UPS failed. Please select value between 1 and 300");
    }
}
