#include <Server/Commands/Commands.h>
#include <Server/Server.h>
#include <Server/Staff.h>
#include <Util/Alloc.h>
#include <Util/Checks/PlayerChecks.h>
#include <Util/Checks/TimeChecks.h>
#include <Util/Log.h>
#include <Util/Nanos.h>
#include <Util/Notice.h>
#include <Util/Uthash.h>
#include <ctype.h>
#include <string.h>

void receive_handle_send_message(server_t* server, player_t* player, stream_t* data)
{
    if (server->protocol.num_players == 0) {
        return;
    }
    uint32_t packet_size = data->length;
    uint8_t  received_id = stream_read_u8(data);
    int      meant_for   = stream_read_u8(data);
    uint32_t length      = stream_left(data);
    if (length > 2048) {
        length = 2048; // Lets limit messages to 2048 characters
    }
    char* message = spadesx_calloc(
    length + 1, sizeof(char)); // Allocated 1 more byte in the case that client sent us non null ending string
    stream_read_array(data, message, length);
    if (message[length - 1] != '\0') {
        message[length] = '\0';
        length++;
        packet_size = length + 3;
    }
    if (player->id != received_id) {
        LOG_WARNING("Assigned ID: %d doesnt match sent ID: %d in message packet", player->id, received_id);
    }
    uint8_t reset_time = 1;
    if (!diff_is_older_then_dont_update(
        get_nanos(), player->timers.since_last_message_from_spam, (uint64_t) NANO_IN_MILLI * 2000) &&
        !player->muted && player->permissions <= 1)
    {
        player->spam_counter++;
        if (player->spam_counter >= 5) {
            send_message_to_staff(
            server, "WARNING: Player %s (#%d) is trying to spam. Muting.", player->name, player->id);
            send_server_notice(player,
                               0,
                               "SERVER: You have been muted for excessive spam. If you feel like this is a mistake "
                               "contact staff via /admin command");
            player->muted        = 1;
            player->spam_counter = 0;
        }
        reset_time = 0;
    }
    if (reset_time) {
        player->timers.since_last_message_from_spam = get_nanos();
        player->spam_counter                        = 0;
    }

    if (!diff_is_older_then(get_nanos(), &player->timers.since_last_message, (uint64_t) NANO_IN_MILLI * 400) &&
        player->permissions <= 1)
    {
        send_server_notice(player, 0, "WARNING: You sent last message too fast and thus was not sent out to players");
        return;
    }

    for (unsigned long index = 0; index < strlen(message); ++index) {
        if (isgraph(message[index]) == 0 && message[index] != ' ' && message[index] > '\b') {
            send_server_notice(
            player, 0, "WARNING: The message you sent contained unreadable characters and thus was ignored");
            LOG_WARNING("Player %s (#%hhu) tried to send message containing unreadable character. Message ignored",
                        player->name,
                        player->id);
            return;
        }
    }

    char meantFor[7];
    switch (meant_for) {
        case 0:
            snprintf(meantFor, 7, "Global");
            break;
        case 1:
            snprintf(meantFor, 5, "Team");
            break;
        case 2:
            snprintf(meantFor, 7, "System");
            break;
    }
    LOG_INFO("Player %s (#%hhu) (%s) said: %s", player->name, player->id, meantFor, message);

    uint8_t sent = 0;
    if (message[0] == '/') {
        command_handle(server, player, message, 0);
    } else {
        if (!player->muted) {
            ENetPacket* packet = enet_packet_create(NULL, packet_size, ENET_PACKET_FLAG_RELIABLE);
            stream_t    stream = {packet->data, packet->dataLength, 0};
            stream_write_u8(&stream, PACKET_TYPE_CHAT_MESSAGE);
            stream_write_u8(&stream, player->id);
            stream_write_u8(&stream, meant_for);
            stream_write_array(&stream, message, length);
            player_t *connected_player, *tmp;
            HASH_ITER(hh, server->players, connected_player, tmp)
            {
                if (is_past_join_screen(connected_player) && !player->muted &&
                    ((connected_player->team == player->team && meant_for == TEAM_B) || meant_for == TEAM_A))
                {
                    if (enet_peer_send(connected_player->peer, 0, packet) == 0) {
                        sent = 1;
                    }
                }
            }
            if (sent == 0) {
                enet_packet_destroy(packet);
            }
        }
    }
    free(message);
}
