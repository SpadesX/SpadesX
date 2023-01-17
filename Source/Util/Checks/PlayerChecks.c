#include <Server/Structs/PlayerStruct.h>
#include <math.h>

uint8_t player_to_player_visibile(player_t* player, player_t* player2)
{
    float distance = 0;
    distance       = sqrt(fabs(pow((player->movement.position.x - player2->movement.position.x), 2)) +
                    fabs(pow((player->movement.position.y - player2->movement.position.y), 2)));
    if (player->team == TEAM_SPECTATOR) {
        return 1;
    } else if (distance >= 132) {
        return 0;
    } else {
        return 1;
    }
}

uint8_t is_past_state_data(player_t* player)
{
    if (player->state == STATE_PICK_SCREEN || player->state == STATE_SPAWNING ||
        player->state == STATE_WAITING_FOR_RESPAWN || player->state == STATE_READY)
    {
        return 1;
    } else {
        return 0;
    }
}

uint8_t is_past_join_screen(player_t* player)
{
    if (player->state == STATE_SPAWNING || player->state == STATE_WAITING_FOR_RESPAWN || player->state == STATE_READY) {
        return 1;
    } else {
        return 0;
    }
}
