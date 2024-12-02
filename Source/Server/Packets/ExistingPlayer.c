#include <Server/Packets/Packets.h>
#include <Server/ParseConvert.h>
#include <Server/Server.h>
#include <Util/Alloc.h>
#include <Util/Checks/PlayerChecks.h>
#include <Util/Enums.h>
#include <Util/Log.h>
#include <Util/Notice.h>
#include <Util/Uthash.h>
#include <Util/Utlist.h>
#include <Util/Weapon.h>
#include <ctype.h>

#define PLAYER_NAME_STRLEN 16

void send_existing_player(server_t* server, player_t* receiver, player_t* existing_player)
{
    if (server->protocol.num_players == 0) {
        return;
    }
    ENetPacket* packet = enet_packet_create(NULL, 28, ENET_PACKET_FLAG_RELIABLE);
    stream_t    stream = {packet->data, packet->dataLength, 0};
    stream_write_u8(&stream, PACKET_TYPE_EXISTING_PLAYER);
    stream_write_u8(&stream, existing_player->id);                // ID
    stream_write_u8(&stream, existing_player->team);              // TEAM
    stream_write_u8(&stream, existing_player->weapon);            // WEAPON
    stream_write_u8(&stream, existing_player->item);              // HELD ITEM
    stream_write_u32(&stream, existing_player->kills);            // KILLS
    stream_write_color_rgb(&stream, existing_player->tool_color); // COLOR
    stream_write_array(&stream, existing_player->name, PLAYER_NAME_STRLEN);       // NAME

    if (enet_peer_send(receiver->peer, 0, packet) != 0) {
        LOG_WARNING("Failed to send player state");
        enet_packet_destroy(packet);
    }
}

void receive_existing_player(server_t* server, player_t* player, stream_t* data)
{
    if (player->team != TEAM_SPECTATOR) {
        return;
    }
    stream_skip(data, 1); // Clients always send a "dumb" ID here since server has not sent them their ID yet
    player->team   = stream_read_u8(data);
    player->weapon = stream_read_u8(data);
    player->item   = stream_read_u8(data);
    player->kills  = stream_read_u32(data);

    if (player->team != TEAM_A && player->team != TEAM_B && player->team != TEAM_SPECTATOR) {
        LOG_WARNING("Player %s (#%hhu) sent invalid team. Switching them to Spectator", player->name, player->id);
        player->team = TEAM_SPECTATOR;
    }

    if (player->team != TEAM_SPECTATOR) {
        server->protocol.num_team_users[player->team]++;
    }

    stream_read_color_rgb(data);
    player->ups = 60;

    uint32_t length  = stream_left(data);
    uint8_t  invName = 0;
    if (length > PLAYER_NAME_STRLEN) {
        LOG_WARNING("Name of player %d is too long. Cutting", player->id);
        length = PLAYER_NAME_STRLEN;
    } else {
        player->name[length] = '\0';
    }
    stream_read_array(data, player->name, length);

    if (strlen(player->name) == 0) {
        snprintf(player->name, strlen("Deuce") + 1, "Deuce");
        invName = 1;
    } else if (player->name[0] == '#') {
        snprintf(player->name, strlen("Deuce") + 1, "Deuce");
        invName = 1;
    }

    char* lowerCaseName = spadesx_malloc((strlen(player->name) + 1) * sizeof(char));
    snprintf(lowerCaseName, strlen(player->name), "%s", player->name);

    for (uint32_t i = 0; i < strlen(player->name); ++i)
        lowerCaseName[i] = tolower(lowerCaseName[i]);

    char* unwantedNames[] = {"igger", "1gger", "igg3r", "1gg3r", NULL};

    for (unsigned long index = 0; index < strlen(lowerCaseName); ++index) {
        if (isgraph(lowerCaseName[index]) == 0 && lowerCaseName[index] != ' ') {
            snprintf(player->name, strlen("Deuce") + 1, "Deuce");
            invName = 1;
        }
    }

    int index = 0;

    while (unwantedNames[index] != NULL) {
        if (strstr(lowerCaseName, unwantedNames[index]) != NULL &&
            strcmp(unwantedNames[index], strstr(lowerCaseName, unwantedNames[index])) == 0)
        {
            snprintf(player->name, strlen("Deuce") + 1, "Deuce");
            free(lowerCaseName);
            return;
        }
        index++;
    }

    free(lowerCaseName);

    // ensure the player has a unique name.
    int       found_match = 0;
    player_t *connected_player, *tmp;
    char      new_name[PLAYER_NAME_STRLEN + 1] = "";
    strcpy(new_name, player->name);
    while (1) {
        found_match = 0;
        HASH_ITER(hh, server->players, connected_player, tmp)
        {
            if (!is_past_join_screen(connected_player) || connected_player->id == player->id) {
                // nonapplicable
                continue;
            }

            if (strcmp(new_name, connected_player->name) == 0) {
                // found match
                found_match = 1;
            }
        }

        if (!found_match) {
            // non-conflicting name. we're done here.
            break;
        }

        new_name[0] = '\0';
        strcpy(new_name, player->name);
        
        char id_str[4];
        snprintf(id_str, 4, "%d", player->id + 15);

        size_t name_len = strlen(new_name);
        size_t id_len = strlen(id_str);
        if (name_len + id_len >= PLAYER_NAME_STRLEN) {
            name_len = PLAYER_NAME_STRLEN - id_len;
            new_name[name_len] = '\0';
        }
        strncat(new_name, id_str, id_len);
    }
    player->name[0] = '\0';
    strcpy(player->name, new_name);

    set_default_player_ammo(player);
    player->state = STATE_SPAWNING;
    char IP[17];
    format_ip_to_str(IP, player->ip);
    char team[15];
    team_id_to_str(server, team, player->team);
    LOG_INFO("Player %s (%s, #%hhu) joined %s", player->name, IP, player->id, team);
    if (player->welcome_sent == 0) {
        string_node_t* welcomeMessage;
        DL_FOREACH(server->welcome_messages, welcomeMessage)
        {
            send_server_notice(player, 0, welcomeMessage->string);
        }
        if (invName) {
            send_server_notice(player,
                               0,
                               "Your name was either empty, had # in front of it, had unreadable character in it or "
                               "contained something nasty. Your name "
                               "has been set to %s",
                               player->name);
        }
        player->welcome_sent = 1; // So we dont send the message to the player on each time they spawn.
    }

    if (server->protocol.gamemode.intel_held[0] == 0) {
        send_move_object(server, 0, 0, server->protocol.gamemode.intel[0]);
    }
    if (server->protocol.gamemode.intel_held[1] == 0) {
        send_move_object(server, 1, 1, server->protocol.gamemode.intel[1]);
    }
}
