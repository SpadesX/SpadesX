#include <Server/Structs/ServerStruct.h>
#include <math.h>

uint8_t player_to_player_visibile(server_t* server, uint8_t player_id, uint8_t player_id2)
{
    float distance = 0;
    distance       = sqrt(
    fabs(pow((server->player[player_id].movement.position.x - server->player[player_id2].movement.position.x), 2)) +
    fabs(pow((server->player[player_id].movement.position.y - server->player[player_id2].movement.position.y), 2)));
    if (server->player[player_id].team == TEAM_SPECTATOR) {
        return 1;
    } else if (distance >= 132) {
        return 0;
    } else {
        return 1;
    }
}

uint8_t is_past_state_data(server_t* server, uint8_t player_id)
{
    player_t player = server->player[player_id];
    if (player.state == STATE_PICK_SCREEN || player.state == STATE_SPAWNING ||
        player.state == STATE_WAITING_FOR_RESPAWN || player.state == STATE_READY)
    {
        return 1;
    } else {
        return 0;
    }
}

uint8_t is_past_join_screen(server_t* server, uint8_t player_id)
{
    player_t player = server->player[player_id];
    if (player.state == STATE_SPAWNING || player.state == STATE_WAITING_FOR_RESPAWN || player.state == STATE_READY) {
        return 1;
    } else {
        return 0;
    }
}
