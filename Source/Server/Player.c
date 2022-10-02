#include <Server/Grenade.h>
#include <Server/IntelTent.h>
#include <Server/Master.h>
#include <Server/Packets/Packets.h>
#include <Server/ParseConvert.h>
#include <Server/Structs/ServerStruct.h>
#include <Util/Checks/PlayerChecks.h>
#include <Util/Checks/PositionChecks.h>
#include <Util/Checks/TimeChecks.h>
#include <Util/JSONHelpers.h>
#include <Util/Nanos.h>
#include <Util/Notice.h>
#include <Util/Physics.h>
#include <Util/Utlist.h>
#include <enet/enet.h>
#include <json-c/json_util.h>
#include <math.h>
#include <time.h>

static uint8_t _on_connect(server_t* server)
{
    if (server->protocol.num_players == server->protocol.max_players) {
        return 0xFF;
    }
    uint8_t player_id;
    for (player_id = 0; player_id < server->protocol.max_players; ++player_id) {
        if (server->player[player_id].state == STATE_DISCONNECTED) {
            server->player[player_id].state = STATE_STARTING_MAP;
            break;
        }
    }
    server->protocol.num_players++;
    return player_id;
}

void update_movement_and_grenades(server_t* server)
{
    physics_set_globals((server->global_timers.update_time - server->global_timers.time_since_start) / 1000000000.f,
                        (server->global_timers.update_time - server->global_timers.last_update_time) / 1000000000.f);
    for (int player_id = 0; player_id < server->protocol.max_players; player_id++) {
        if (server->player[player_id].state == STATE_READY) {
            long falldamage = 0;
            falldamage      = physics_move_player(server, player_id);
            if (falldamage > 0) {
                vector3f_t zero = {0, 0, 0};
                send_set_hp(server, player_id, player_id, falldamage, 0, 4, 5, 0, zero);
            }
            if (server->protocol.current_gamemode != GAME_MODE_ARENA) {
                handleTentAndIntel(server, player_id);
            }
        }
        handle_grenade(server, player_id);
    }
}

uint8_t get_player_unstuck(server_t* server, uint8_t player_id)
{
    for (float z = server->player[player_id].movement.prev_legit_pos.z - 1;
         z <= server->player[player_id].movement.prev_legit_pos.z + 1;
         z++)
    {
        if (valid_pos_3f(server,
                         player_id,
                         server->player[player_id].movement.prev_legit_pos.x,
                         server->player[player_id].movement.prev_legit_pos.y,
                         z))
        {
            server->player[player_id].movement.prev_legit_pos.z = z;
            server->player[player_id].movement.position         = server->player[player_id].movement.prev_legit_pos;
            return 1;
        }
        for (float x = server->player[player_id].movement.prev_legit_pos.x - 1;
             x <= server->player[player_id].movement.prev_legit_pos.x + 1;
             x++)
        {
            for (float y = server->player[player_id].movement.prev_legit_pos.y - 1;
                 y <= server->player[player_id].movement.prev_legit_pos.y + 1;
                 y++)
            {
                if (valid_pos_3f(server, player_id, x, y, z)) {
                    server->player[player_id].movement.prev_legit_pos.x = x;
                    server->player[player_id].movement.prev_legit_pos.y = y;
                    server->player[player_id].movement.prev_legit_pos.z = z;
                    server->player[player_id].movement.position = server->player[player_id].movement.prev_legit_pos;
                    return 1;
                }
            }
        }
    }
    return 0;
}

void set_player_respawn_point(server_t* server, uint8_t player_id)
{
    if (server->player[player_id].team != TEAM_SPECTATOR) {
        quad3d_t* spawn = server->protocol.spawns + server->player[player_id].team;

        float dx = spawn->to.x - spawn->from.x;
        float dy = spawn->to.y - spawn->from.y;

        server->player[player_id].movement.position.x = spawn->from.x + dx * ((float) rand() / (float) RAND_MAX);
        server->player[player_id].movement.position.y = spawn->from.y + dy * ((float) rand() / (float) RAND_MAX);
        server->player[player_id].movement.position.z =
        mapvxl_find_top_block(&server->s_map.map,
                              server->player[player_id].movement.position.x,
                              server->player[player_id].movement.position.y) -
        2.36f;

        server->player[player_id].movement.forward_orientation.x = 0.f;
        server->player[player_id].movement.forward_orientation.y = 0.f;
        server->player[player_id].movement.forward_orientation.z = 0.f;
    }
}

void init_player(server_t*  server,
                 uint8_t    player_id,
                 uint8_t    reset,
                 uint8_t    disconnect,
                 vector3f_t empty,
                 vector3f_t forward,
                 vector3f_t strafe,
                 vector3f_t height)
{
    if (reset == 0) {
        server->player[player_id].state  = STATE_DISCONNECTED;
        server->player[player_id].queues = NULL;
    }
    server->player[player_id].ups                          = 60;
    server->player[player_id].timers.time_since_last_wu    = get_nanos();
    server->player[player_id].input                        = 0;
    server->player[player_id].movement.eye_pos             = empty;
    server->player[player_id].movement.forward_orientation = forward;
    server->player[player_id].movement.strafe_orientation  = strafe;
    server->player[player_id].movement.height_orientation  = height;
    server->player[player_id].movement.position            = empty;
    server->player[player_id].movement.velocity            = empty;
    if (reset == 0 && disconnect == 0) {
        permissions_t roleList[5] = {{"manager", &server->manager_passwd, 4},
                                     {"admin", &server->admin_passwd, 3},
                                     {"mod", &server->mod_passwd, 2},
                                     {"guard", &server->guard_passwd, 1},
                                     {"trusted", &server->trusted_passwd, 0}};
        for (unsigned long x = 0; x < sizeof(roleList) / sizeof(permissions_t); ++x) {
            server->player[player_id].role_list[x] = roleList[x];
        }
        server->player[player_id].grenade = NULL;
    }
    server->player[player_id].airborne                             = 0;
    server->player[player_id].wade                                 = 0;
    server->player[player_id].lastclimb                            = 0;
    server->player[player_id].move_backwards                       = 0;
    server->player[player_id].move_forward                         = 0;
    server->player[player_id].move_left                            = 0;
    server->player[player_id].move_right                           = 0;
    server->player[player_id].jumping                              = 0;
    server->player[player_id].crouching                            = 0;
    server->player[player_id].sneaking                             = 0;
    server->player[player_id].sprinting                            = 0;
    server->player[player_id].primary_fire                         = 0;
    server->player[player_id].secondary_fire                       = 0;
    server->player[player_id].can_build                            = 1;
    server->player[player_id].allow_killing                        = 1;
    server->player[player_id].allow_team_killing                   = 0;
    server->player[player_id].muted                                = 0;
    server->player[player_id].told_to_master                       = 0;
    server->player[player_id].timers.since_last_base_enter         = 0;
    server->player[player_id].timers.since_last_base_enter_restock = 0;
    server->player[player_id].timers.since_last_3block_dest        = 0;
    server->player[player_id].timers.since_last_block_dest         = 0;
    server->player[player_id].timers.since_last_block_plac         = 0;
    server->player[player_id].timers.since_last_grenade_thrown     = 0;
    server->player[player_id].timers.since_last_shot               = 0;
    server->player[player_id].timers.time_since_last_wu            = 0;
    server->player[player_id].timers.since_last_weapon_input       = 0;
    server->player[player_id].hp                                   = 100;
    server->player[player_id].blocks                               = 50;
    server->player[player_id].grenades                             = 3;
    server->player[player_id].has_intel                            = 0;
    server->player[player_id].reloading                            = 0;
    server->player[player_id].client                               = ' ';
    server->player[player_id].version_minor                        = 0;
    server->player[player_id].version_major                        = 0;
    server->player[player_id].version_revision                     = 0;
    server->player[player_id].periodic_delay_index                 = 0;
    server->player[player_id].current_periodic_message             = server->periodic_messages;
    server->player[player_id].welcome_sent                         = 0;
    if (reset == 0) {
        server->player[player_id].permissions = 0;
    } else if (reset == 1) {
        grenade_t* grenade;
        grenade_t* tmp;
        DL_FOREACH_SAFE(server->player[player_id].grenade, grenade, tmp)
        {
            DL_DELETE(server->player[player_id].grenade, grenade);
            free(grenade);
        }
    }
    server->player[player_id].is_invisible = 0;
    server->player[player_id].kills        = 0;
    server->player[player_id].deaths       = 0;
    memset(server->player[player_id].name, 0, 17);
    memset(server->player[player_id].os_info, 0, 255);
}

void send_joining_data(server_t* server, uint8_t player_id)
{
    LOG_INFO("Sending state to %s (#%hhu)", server->player[player_id].name, player_id);
    for (uint8_t i = 0; i < server->protocol.max_players; ++i) {
        if (i != player_id && is_past_join_screen(server, i)) {
            send_existing_player(server, player_id, i);
        }
    }
    send_state_data(server, player_id);
}

void on_player_update(server_t* server, uint8_t player_id)
{
    switch (server->player[player_id].state) {
        case STATE_STARTING_MAP:
            server->player[player_id].blockBuffer = NULL;
            send_map_start(server, player_id);
            break;
        case STATE_LOADING_CHUNKS:
            send_map_chunks(server, player_id);
            break;
        case STATE_JOINING:
            send_joining_data(server, player_id);
            break;
        case STATE_SPAWNING:
            server->player[player_id].hp             = 100;
            server->player[player_id].grenades       = 3;
            server->player[player_id].blocks         = 50;
            server->player[player_id].item           = 2;
            server->player[player_id].input          = 0;
            server->player[player_id].move_forward   = 0;
            server->player[player_id].move_backwards = 0;
            server->player[player_id].move_left      = 0;
            server->player[player_id].move_right     = 0;
            server->player[player_id].jumping        = 0;
            server->player[player_id].crouching      = 0;
            server->player[player_id].sneaking       = 0;
            server->player[player_id].sprinting      = 0;
            server->player[player_id].primary_fire   = 0;
            server->player[player_id].secondary_fire = 0;
            server->player[player_id].alive          = 1;
            server->player[player_id].reloading      = 0;
            set_player_respawn_point(server, player_id);
            send_respawn(server, player_id);
            LOG_INFO("Player %s (#%hhu) spawning at: %f %f %f",
                     server->player[player_id].name,
                     player_id,
                     server->player[player_id].movement.position.x,
                     server->player[player_id].movement.position.y,
                     server->player[player_id].movement.position.z);
            break;
        case STATE_WAITING_FOR_RESPAWN:
        {
            if (time(NULL) - server->player[player_id].timers.start_of_respawn_wait >=
                server->player[player_id].respawn_time)
            {
                server->player[player_id].state = STATE_SPAWNING;
            }
            break;
        }
        case STATE_READY:
            // send data
            if (server->master.enable_master_connection == 1) {
                if (server->player[player_id].told_to_master == 0) {
                    master_update(server);
                    server->player[player_id].told_to_master = 1;
                }
            }
            break;
        default:
            // disconnected
            break;
    }
}

void on_new_player_connection(server_t* server, ENetEvent* event)
{
    uint8_t banned_user = 0;
    uint8_t player_id;
    if (event->data != VERSION_0_75) {
        enet_peer_disconnect_now(event->peer, REASON_WRONG_PROTOCOL_VERSION);
        return;
    }

    struct json_object* root = json_object_from_file("Bans.json");
    if (root == NULL) {
        FILE* fp;
        fp = fopen("Bans.json", "w+");
        if (fp == NULL) {
            perror("Unable to open/create Bans.json with error: ");
            exit(EXIT_FAILURE);
        }
        fclose(fp);
        root = json_object_new_object();
        json_object_object_add(root, "Bans", json_object_new_array());
        json_object_to_file("Bans.json", root);
    }
    ip_t hostIP;
    hostIP.cidr = 24;
    hostIP.ip32 = event->peer->address.host;
    struct json_object* array;
    json_object_object_get_ex(root, "Bans", &array);
    int count = json_object_array_length(array);
    for (int i = 0; i < count; ++i) {
        struct json_object* object_at_index = json_object_array_get_idx(array, i);
        const char*         IP;
        const char*         start_of_range_string;
        const char*         end_of_range_string;
        READ_STR_FROM_JSON(object_at_index, start_of_range_string, start_of_range, "start of range", "0.0.0.0", 1);
        READ_STR_FROM_JSON(object_at_index, end_of_range_string, end_of_range, "end of range", "0.0.0.0", 1);
        READ_STR_FROM_JSON(object_at_index, IP, IP, "IP", "0.0.0.0", 1);
        ip_t ipStruct;
        ip_t start_of_range, end_of_range;
        if (format_str_to_ip((char*) IP, &ipStruct) &&
            (format_str_to_ip((char*) start_of_range_string, &start_of_range) &&
             format_str_to_ip((char*) end_of_range_string, &end_of_range)))
        {
            if (ip_in_range(hostIP, ipStruct, start_of_range, end_of_range)) {
                const char* name_of_player;
                const char* reason;
                double      time    = 0.0f;
                uint64_t    timeNow = get_nanos();
                READ_STR_FROM_JSON(object_at_index, name_of_player, Name, "Name", "Deuce", 0);
                READ_STR_FROM_JSON(object_at_index, reason, Reason, "Reason", "None", 0);
                READ_DOUBLE_FROM_JSON(object_at_index, time, Time, "Time", 0.0f, 0);
                if (((long double) timeNow / NANO_IN_MINUTE) > time && time != 0) {
                    json_object_array_del_idx(array, i, 1);
                    json_object_to_file("Bans.json", root);
                    continue; // Continue searching for bans and delete all the old ones that match host IP
                              // or its range
                } else {
                    enet_peer_disconnect(event->peer, REASON_BANNED);

                    LOG_WARNING("Banned user %s tried to join with IP: %hhu.%hhu.%hhu.%hhu Banned for: %s",
                                name_of_player,
                                hostIP.ip[0],
                                hostIP.ip[1],
                                hostIP.ip[2],
                                hostIP.ip[3],
                                reason);
                    banned_user       = 1;
                    event->peer->data = (void*) ((size_t) server->protocol.max_players - 1);
                    break;
                }
            }
        }
    }
    json_object_put(root);

    if (banned_user) {
        return;
    }
    // check peer
    // ...
    // find next free ID
    player_id = _on_connect(server);
    if (player_id == 0xFF) {
        enet_peer_disconnect_now(event->peer, REASON_SERVER_FULL);
        LOG_WARNING("Server full. Kicking player");
        return;
    }
    server->player[player_id].peer    = event->peer;
    event->peer->data                 = (void*) ((size_t) player_id);
    server->player[player_id].hp      = 100;
    server->player[player_id].ip.ip32 = event->peer->address.host;

    format_ip_to_str(server->player[player_id].name, server->player[player_id].ip);
    snprintf(server->player[player_id].name, 6, "Limbo");
    char ipString[17];
    format_ip_to_str(ipString, server->player[player_id].ip);
    LOG_INFO("Player %s (%s, #%hhu) connected", server->player[player_id].name, ipString, player_id);

    server->player[player_id].timers.since_periodic_message = get_nanos();
    server->player[player_id].current_periodic_message      = server->periodic_messages;
    server->player[player_id].periodic_delay_index          = 0;
}

void for_players(server_t* server)
{
    for (int player_id = 0; player_id < server->protocol.max_players; ++player_id) {
        if (is_past_join_screen(server, player_id)) {
            uint64_t timeNow = get_nanos();
            if (server->player[player_id].primary_fire == 1 && server->player[player_id].reloading == 0) {
                if (server->player[player_id].weapon_clip > 0) {
                    uint64_t milliseconds = 0;
                    switch (server->player[player_id].weapon) {
                        case WEAPON_RIFLE:
                        {
                            milliseconds = 500;
                            break;
                        }
                        case WEAPON_SMG:
                        {
                            milliseconds = 100;
                            break;
                        }
                        case WEAPON_SHOTGUN:
                        {
                            milliseconds = 1000;
                            break;
                        }
                    }
                    if (diff_is_older_then(timeNow,
                                           &server->player[player_id].timers.since_last_weapon_input,
                                           NANO_IN_MILLI * milliseconds))
                    {
                        server->player[player_id].weapon_clip--;
                    }
                }
            } else if (server->player[player_id].primary_fire == 0 && server->player[player_id].reloading == 1) {
                switch (server->player[player_id].weapon) {
                    case WEAPON_RIFLE:
                    case WEAPON_SMG:
                    {
                        if (diff_is_older_then(timeNow,
                                               &server->player[player_id].timers.since_reload_start,
                                               NANO_IN_MILLI * (uint64_t) 2500))
                        {
                            uint8_t defaultAmmo = RIFLE_DEFAULT_CLIP;
                            if (server->player[player_id].weapon == WEAPON_SMG) {
                                defaultAmmo = SMG_DEFAULT_CLIP;
                            }
                            double newReserve = fmax(0,
                                                     server->player[player_id].weapon_reserve -
                                                     (defaultAmmo - server->player[player_id].weapon_clip));
                            server->player[player_id].weapon_clip +=
                            server->player[player_id].weapon_reserve - newReserve;
                            server->player[player_id].weapon_reserve = newReserve;
                            server->player[player_id].reloading      = 0;
                            send_weapon_reload(server, player_id, 0, 0, 0);
                        }
                        break;
                    }
                    case WEAPON_SHOTGUN:
                    {
                        if (diff_is_older_then(timeNow,
                                               &server->player[player_id].timers.since_reload_start,
                                               NANO_IN_MILLI * (uint64_t) 500))
                        {
                            if (server->player[player_id].weapon_reserve == 0 ||
                                server->player[player_id].weapon_clip == SHOTGUN_DEFAULT_CLIP)
                            {
                                server->player[player_id].reloading = 0;
                                break;
                            }
                            server->player[player_id].weapon_clip++;
                            server->player[player_id].weapon_reserve--;
                            if (server->player[player_id].weapon_clip == SHOTGUN_DEFAULT_CLIP) {
                                server->player[player_id].reloading = 0;
                            }
                            send_weapon_reload(server, player_id, 0, 0, 0);
                        }
                        break;
                    }
                }
            }
            if (diff_is_older_then(
                timeNow,
                &server->player[player_id].timers.since_periodic_message,
                (uint64_t) (server->periodic_delays[server->player[player_id].periodic_delay_index] * 60) *
                NANO_IN_SECOND))
            {
                string_node_t* message;
                DL_FOREACH(server->periodic_messages, message)
                {
                    send_server_notice(server, player_id, 0, message->string);
                }
                server->player[player_id].periodic_delay_index =
                fmin(server->player[player_id].periodic_delay_index + 1, 4);
            }
        } else if (server->player[player_id].state == STATE_PICK_SCREEN) {
            block_node_t *node, *tmp;
            LL_FOREACH_SAFE(server->player[player_id].blockBuffer, node, tmp)
            {
                if (node->type == 10) {
                    send_set_color_to_player(server, node->sender_id, player_id, node->color);
                    block_line_to_player(server, node->sender_id, player_id, node->position, node->position_end);
                    send_set_color_to_player(server, node->sender_id, player_id, server->player[node->sender_id].color);
                } else {
                    send_set_color_to_player(server, node->sender_id, player_id, node->color);
                    send_block_action_to_player(server,
                                                node->sender_id,
                                                player_id,
                                                node->type,
                                                node->position.x,
                                                node->position.y,
                                                node->position.z);
                    send_set_color_to_player(server, node->sender_id, player_id, server->player[node->sender_id].color);
                }
                LL_DELETE(server->player[player_id].blockBuffer, node);
                free(node);
            }
        } else if (is_past_state_data(server, player_id)) {
        }
    }
}
