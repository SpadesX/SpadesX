#include <Server/Structs/PlayerStruct.h>
#include <Util/Enums.h>
#include <Util/Weapon.h>

void set_default_player_ammo(player_t* player)
{
    switch (player->weapon) {
        case WEAPON_RIFLE:
            player->weapon_reserve  = 50;
            player->weapon_clip     = 10;
            player->default_clip    = RIFLE_DEFAULT_CLIP;
            player->default_reserve = RIFLE_DEFAULT_RESERVE;
            break;
        case WEAPON_SMG:
            player->weapon_reserve  = 120;
            player->weapon_clip     = 30;
            player->default_clip    = SMG_DEFAULT_CLIP;
            player->default_reserve = SMG_DEFAULT_RESERVE;
            break;
        case WEAPON_SHOTGUN:
            player->weapon_reserve  = 48;
            player->weapon_clip     = 6;
            player->default_clip    = SHOTGUN_DEFAULT_CLIP;
            player->default_reserve = SHOTGUN_DEFAULT_RESERVE;
            break;
    }
}

void set_default_player_ammo_reserve(player_t* player)
{
    switch (player->weapon) {
        case WEAPON_RIFLE:
            player->weapon_reserve = 50;
            break;
        case WEAPON_SMG:
            player->weapon_reserve = 120;
            break;
        case WEAPON_SHOTGUN:
            player->weapon_reserve = 48;
            break;
    }
}

inline void set_default_player_pellets(player_t* player)
{
    uint8_t pellets = 0;
    switch (player->weapon) {
        case WEAPON_RIFLE:
        case WEAPON_SMG:
            pellets = 1;
            break;
        case WEAPON_SHOTGUN:
            pellets = 8;
            break;
    }

    if(player->weapon_pellets != pellets) {
        player->weapon_pellets = pellets;
    }
}

uint64_t get_player_weapon_delay_nano(player_t* player)
{
    uint64_t delay = 0;
    switch (player->weapon) {
        case WEAPON_RIFLE:
        {
            delay = NANO_IN_MILLI * 500;
            break;
        }
        case WEAPON_SMG:
        {
            delay = NANO_IN_MILLI * 100;
            break;
        }
        case WEAPON_SHOTGUN:
        {
            delay = NANO_IN_MILLI * 1000;
            break;
        }
    }
    return delay;
}
