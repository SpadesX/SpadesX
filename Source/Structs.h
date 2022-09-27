// Copyright DarkNeutrino 2021
#ifndef STRUCTS_H
#define STRUCTS_H

#include "../Extern/libmapvxl/libmapvxl.h"
#include "Util/Enums.h"
#include "Util/Queue.h"
#include "Util/Types.h"
#include "Util/Uthash.h"
#include "Util/Utlist.h"

#include <enet/enet.h>
#include <signal.h>
#include <sys/types.h>
#include <time.h>

#define VERSION \
    "Beta 1.0"  \
    " Compiled on " __DATE__ " " __TIME__
// #define DEBUG

typedef struct
{
    union
    {
        uint8_t  ip[4];
        uint32_t ip32;
    };
    uint8_t cidr;
} ip_t;

typedef struct permissions
{
    char         access_level[32];
    const char** access_password;
    uint8_t      perm_level_offset;
} permissions_t;

typedef struct grenade
{
    uint8_t         sent;
    float           fuse;
    uint8_t         exploded;
    vector3f_t      position;
    vector3f_t      velocity;
    uint64_t        time_since_sent;
    struct grenade *next, *prev;
} grenade_t;

typedef struct movement
{ // Seriously what the F. Thank voxlap motion for this mess.
    vector3f_t position;
    vector3f_t prev_position;
    vector3f_t prev_legit_pos;
    vector3f_t eye_pos;
    vector3f_t velocity;
    vector3f_t strafe_orientation;
    vector3f_t height_orientation;
    vector3f_t forward_orientation;
    vector3f_t previous_orientation;
} movement_t;

typedef struct orientation
{ // Yet again thanks voxlap.
    vector3f_t forward;
    vector3f_t strafe;
    vector3f_t height;
} orientation_t;

typedef struct string_node
{
    char*               string;
    struct string_node *next, *prev;
} string_node_t;

typedef struct map
{
    uint8_t        map_count;
    string_node_t* current_map;
    // compressed map
    queue_t*       compressed_map;
    uint32_t       compressed_size;
    vector3i_t     result_line[50];
    size_t         map_size;
    mapvxl_t       map;
    string_node_t* map_list;
} map_t;

typedef struct gamemode_vars
{
    uint8_t      score[2];
    uint8_t      score_limit;
    intel_flag_t intel_flags;
    vector3f_t   intel[2];
    vector3f_t   base[2];
    uint8_t      player_intel_team[2];
    uint8_t      intel_held[2];
} gamemode_vars_t;

typedef struct protocol
{
    uint8_t num_users;
    uint8_t num_team_users[2];
    //
    uint8_t num_players;
    uint8_t max_players;
    //
    color_t         color_fog;
    color_t         color_team[2];
    char            name_team[2][11];
    gamemode_t      current_gamemode;
    gamemode_vars_t gamemode;
    // respawn area
    quad3d_t spawns[2];
    uint32_t input_flags;
} protocol_t;

typedef struct timers
{
    uint64_t time_since_last_wu;
    uint64_t since_last_base_enter;
    uint64_t since_last_base_enter_restock;
    uint64_t start_of_respawn_wait;
    uint64_t since_last_shot;
    uint64_t since_last_block_dest_with_gun;
    uint64_t since_last_block_dest;
    uint64_t since_last_block_plac;
    uint64_t since_last_3block_dest;
    uint64_t since_last_grenade_thrown;
    uint64_t since_last_weapon_input;
    uint64_t since_reload_start;
    uint64_t since_last_primary_weapon_input;
    uint64_t since_last_message_from_spam;
    uint64_t since_last_message;
    uint64_t since_possible_spade_nade;
    uint64_t since_periodic_message;
    uint64_t during_noclip_period;
} timers_t;

typedef struct global_timers
{
    uint64_t update_time;
    uint64_t last_update_time;
    uint64_t time_since_start;
    float    time_since_start_simulated;
} global_timers_t;

typedef struct block_node
{
    struct block_node* next;
    vector3i_t         position;
    vector3i_t         position_end;
    color_t            color;
    uint8_t            type;
    uint8_t            sender_id;
} block_node_t;

typedef struct player
{
    queue_t*                 queues;
    ENetPeer*                peer;
    grenade_t*               grenade;
    uint64_t                 permissions;
    string_node_t*           current_periodic_message;
    block_node_t*            blockBuffer;
    timers_t                 timers;
    permissions_t            role_list[5]; // Change me based on the number of access levels you require
    state_t                  state;
    weapon_t                 weapon;
    team_t                   team;
    tool_t                   item;
    weapon_default_clip_t    default_clip;
    weapon_default_reserve_t default_reserve;
    uint32_t                 kills;
    uint32_t                 deaths;
    color_t                  color;
    int                      version_minor;
    int                      version_major;
    int                      version_revision;
    color_t                  tool_color;
    int                      hp;
    float                    lastclimb;
    ip_t                     ip;
    vector3f_t               locAtClick;
    movement_t               movement;
    vector3i_t               result_line[50];
    uint16_t                 ups;
    char                     client;
    uint8_t                  respawn_time;
    uint8_t                  grenades;
    uint8_t                  blocks;
    uint8_t                  weapon_reserve;
    uint8_t                  weapon_clip;
    uint8_t                  alive;
    uint8_t                  input;
    uint8_t                  allow_killing;
    uint8_t                  allow_team_killing;
    uint8_t                  muted;
    uint8_t                  admin_muted;
    uint8_t                  can_build;
    uint8_t                  told_to_master;
    uint8_t                  has_intel;
    uint8_t                  is_invisible;
    uint8_t                  welcome_sent;
    uint8_t                  reloading;
    uint8_t                  spam_counter;
    uint8_t                  periodic_delay_index;
    uint8_t                  invalid_pos_count;
    uint8_t                  move_forward;
    uint8_t                  move_backwards;
    uint8_t                  move_left;
    uint8_t                  move_right;
    uint8_t                  jumping;
    uint8_t                  crouching;
    uint8_t                  sneaking;
    uint8_t                  sprinting;
    uint8_t                  primary_fire;
    uint8_t                  secondary_fire;
    uint8_t                  airborne;
    uint8_t                  wade;
    char                     name[17];
    char                     os_info[255];
} player_t;

typedef struct master
{
    // Master
    ENetHost* client;
    ENetPeer* peer;
    ENetEvent event;
    uint8_t   enable_master_connection;
    uint64_t  time_since_last_send;
} master_t;

typedef struct command_args
{
    uint8_t  player_id;
    uint8_t  console;
    uint32_t permissions;
    uint32_t argc;
    char*    argv[32]; // 32 roles commands should be more than enough for anyone
} command_args_t;

typedef struct command
{
    char    id[30];
    uint8_t parse_args; // Should we even bother parsing it? Would be useful for commands whose one and only
                        // argument is a message, or which don't even have any arguments
    void (*execute)(void* p_server, command_args_t arguments);
    uint32_t        permissions; // 32 roles should be more then enough for anyone
    char            description[1024];
    UT_hash_handle  hh;
    struct command* next;
} command_t;

// Command but with removed hash map and linked list stuff for easy managment
typedef struct command_manager
{
    char    id[30];
    uint8_t parse_args; // Should we even bother parsing it? Would be useful for commands whose one and only
                        // argument is a message, or which don't even have any arguments
    void (*execute)(void* p_server, command_args_t arguments);
    uint32_t permissions; // 32 roles should be more then enough for anyone
    char     description[1024];
} command_manager_t;

typedef struct map_node
{
    int            id;
    vector3i_t     pos;
    uint8_t        visited;
    UT_hash_handle hh;
} map_node_t;

typedef struct server
{
    ENetHost*             host;
    player_t              player[32];
    protocol_t            protocol;
    master_t              master;
    uint16_t              port;
    map_t                 s_map;
    global_timers_t       global_timers;
    command_t*            cmds_map;
    command_t*            cmds_list;
    string_node_t*        welcome_messages;
    uint8_t               welcome_messages_count;
    string_node_t*        periodic_messages;
    uint8_t               periodic_message_count;
    uint8_t*              periodic_delays;
    uint8_t               global_ak;
    uint8_t               global_ab;
    const char*           manager_passwd;
    const char*           admin_passwd;
    const char*           mod_passwd;
    const char*           guard_passwd;
    const char*           trusted_passwd;
    char                  map_name[20];
    char                  server_name[31];
    char                  gamemode_name[7];
    volatile sig_atomic_t running; // volatile keyword is required to have an access to this variable in any thread
} server_t;

#endif
