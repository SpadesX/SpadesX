#ifndef PLAYER_H
#define PLAYER_H

#include <Server/Structs/ServerStruct.h>

void    on_new_player_connection(server_t* server, ENetEvent* event);
int     player_sort(player_t* a, player_t* b);
void    free_all_players(server_t* server);
void    for_players(server_t* server);
void    on_player_update(server_t* server, player_t* player);
void    send_joining_data(server_t* server, player_t* player);
void    init_player(server_t*  server,
                    player_t*  player,
                    uint8_t    reset,
                    uint8_t    disconnect,
                    vector3f_t empty,
                    vector3f_t forward,
                    vector3f_t strafe,
                    vector3f_t height);
void    set_player_respawn_point(server_t* server, player_t* player);
uint8_t get_player_unstuck(server_t* server, player_t* player);
uint8_t is_staff(server_t* server, player_t* player);
void    update_movement_and_grenades(server_t* server);

#endif
