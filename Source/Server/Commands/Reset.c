#include "Util/Uthash.h"
#include <Server/Server.h>
#include <pthread.h>

void cmd_reset(void* p_server, command_args_t arguments)
{
    server_t* server = (server_t*) p_server;
    (void) arguments;
    player_t *connected_player, *tmp;
    HASH_ITER(hh, server->players, connected_player, tmp) {
        if (connected_player->state != STATE_DISCONNECTED) {
            pthread_mutex_lock(&server->mutex.physics);
            connected_player->state = STATE_STARTING_MAP;
            pthread_mutex_unlock(&server->mutex.physics);
        }
    }
    server_reset(server);
}
