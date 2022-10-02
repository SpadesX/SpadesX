#include <Server/Commands/Commands.h>
#include <Server/Structs/ServerStruct.h>
#include <Util/Checks/PlayerChecks.h>
#include <Util/Notice.h>
#include <stdarg.h>
#include <stdio.h>

uint8_t is_staff(server_t* server, uint8_t player_id)
{
    if (player_has_permission(server, player_id, 0, 4294967294)) {
        return 1;
    }
    return 0;
}

void send_message_to_staff(server_t* server, const char* format, ...)
{
    va_list args;
    va_start(args, format);
    char fMessage[1024];
    vsnprintf(fMessage, 1024, format, args);
    va_end(args);

    send_server_notice(server, 32, 1, fMessage);

    for (uint8_t ID = 0; ID < server->protocol.max_players; ++ID) {
        if (is_past_join_screen(server, ID) && is_staff(server, ID)) {
            send_server_notice(server, ID, 0, fMessage);
        }
    }
}
