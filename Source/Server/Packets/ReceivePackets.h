#ifndef RECEIVEPACKETS_H
#define RECEIVEPACKETS_H

#include <Server/Server.h>

void receive_handle_send_message(server_t* server, player_t* player, stream_t* data);
void receive_orientation_data(server_t* server, player_t* player, stream_t* data);
void receive_input_data(server_t* server, player_t* player, stream_t* data);
void receive_hit_packet(server_t* server, player_t* player, stream_t* data);
void receive_grenade_packet(server_t* server, player_t* player, stream_t* data);
void receive_position_data(server_t* server, player_t* player, stream_t* data);
void receive_existing_player(server_t* server, player_t* player, stream_t* data);
void receive_block_action(server_t* server, player_t* player, stream_t* data);
void receive_block_line(server_t* server, player_t* player, stream_t* data);
void receive_set_tool(server_t* server, player_t* player, stream_t* data);
void receive_set_color(server_t* server, player_t* player, stream_t* data);
void receive_weapon_input(server_t* server, player_t* player, stream_t* data);
void receive_weapon_reload(server_t* server, player_t* player, stream_t* data);
void receive_change_team(server_t* server, player_t* player, stream_t* data);
void receive_change_weapon(server_t* server, player_t* player, stream_t* data);
void receive_version_response(server_t* server, player_t* player, stream_t* data);

#endif
