#ifndef COMMANDSMANAGER_H
#define COMMANDSMANAGER_H

#include <Server/Server.h>

void cmd_generate_ban(server_t* server, command_args_t arguments, float time, ip_t ip, char* reason);

void cmd_admin(void* p_server, command_args_t arguments);
void cmd_ban_custom(void* p_server, command_args_t arguments);
void cmd_ban_range(void* p_server, command_args_t arguments);
void cmd_admin_mute(void* p_server, command_args_t arguments);
void cmd_clin(void* p_server, command_args_t arguments);
void cmd_help(void* p_server, command_args_t arguments);
void cmd_intel(void* p_server, command_args_t arguments);
void cmd_inv(void* p_server, command_args_t arguments);
void cmd_kick(void* p_server, command_args_t arguments);
void cmd_kill(void* p_server, command_args_t arguments);
void cmd_login(void* p_server, command_args_t arguments);
void cmd_logout(void* p_server, command_args_t arguments);
void cmd_master(void* p_server, command_args_t arguments);
void cmd_mute(void* p_server, command_args_t arguments);
void cmd_pm(void* p_server, command_args_t arguments);
void cmd_ratio(void* p_server, command_args_t arguments);
void cmd_reset(void* p_server, command_args_t arguments);
void cmd_say(void* p_server, command_args_t arguments);
void cmd_server(void* p_server, command_args_t arguments);
void cmd_toggle_build(void* p_server, command_args_t arguments);
void cmd_toggle_kill(void* p_server, command_args_t arguments);
void cmd_toggle_team_kill(void* p_server, command_args_t arguments);
void cmd_tp(void* p_server, command_args_t arguments);
void cmd_tpc(void* p_server, command_args_t arguments);
void cmd_unban(void* p_server, command_args_t arguments);
void cmd_unban_range(void* p_server, command_args_t arguments);
void cmd_undo_ban(void* p_server, command_args_t arguments);
void cmd_ups(void* p_server, command_args_t arguments);
void cmd_shutdown(void* p_server, command_args_t arguments);

#endif
