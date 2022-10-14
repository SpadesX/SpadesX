#ifndef STARTSTRUCT_H
#define STARTSTRUCT_H

typedef struct server_args
{
    uint16_t       port;
    uint32_t       connections;
    uint32_t       channels;
    uint32_t       in_bandwidth;
    uint32_t       out_bandwidth;
    uint8_t        master;
    string_node_t* map_list;
    uint8_t        map_count;
    string_node_t* welcome_message_list;
    uint8_t        welcome_message_list_len;
    string_node_t* periodic_message_list;
    uint8_t        periodic_message_list_len;
    uint8_t*       periodic_delays;
    const char*    manager_password;
    const char*    admin_password;
    const char*    mod_password;
    const char*    guard_password;
    const char*    trusted_password;
    const char*    server_name;
    const char*    team1_name;
    const char*    team2_name;
    uint8_t*       team1_color;
    uint8_t*       team2_color;
    uint8_t        gamemode;
} server_args;

#endif
