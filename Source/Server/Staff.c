#include <Server/Commands/Commands.h>
#include <Server/Structs/ServerStruct.h>
#include <Util/Checks/PlayerChecks.h>
#include <Util/Notice.h>
#include <stdarg.h>
#include <stdio.h>

uint8_t is_staff(server_t* server, player_t* player)
{
    (void)server;
    if (player_has_permission(player, 0, 4294967294)) {
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

    player_t* null_player = NULL;
    send_server_notice(null_player, 1, fMessage);

    player_t *connected_player, *tmp;
    HASH_ITER(hh, server->players, connected_player, tmp) {
        if (is_past_join_screen(connected_player) && is_staff(server, connected_player)) {
            send_server_notice(connected_player, 0, fMessage);
        }
    }
}
