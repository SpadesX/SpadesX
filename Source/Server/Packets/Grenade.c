#include <Server/Server.h>
#include <Server/Staff.h>
#include <Util/Checks/PacketChecks.h>
#include <Util/Checks/PlayerChecks.h>
#include <Util/Checks/PositionChecks.h>
#include <Util/Checks/TimeChecks.h>
#include <Util/Log.h>
#include <Util/Nanos.h>
#include <Util/Notice.h>
#include <Util/Utlist.h>
#include <math.h>

void send_grenade(server_t* server, player_t* player, float fuse, vector3f_t position, vector3f_t velocity)
{
    if (server->protocol.num_players == 0) {
        return;
    }
    ENetPacket* packet = enet_packet_create(NULL, 30, ENET_PACKET_FLAG_RELIABLE);
    stream_t    stream = {packet->data, packet->dataLength, 0};
    stream_write_u8(&stream, PACKET_TYPE_GRENADE_PACKET);
    stream_write_u8(&stream, player->id);
    stream_write_f(&stream, fuse);
    stream_write_f(&stream, position.x);
    stream_write_f(&stream, position.y);
    stream_write_f(&stream, position.z);
    stream_write_f(&stream, velocity.x);
    stream_write_f(&stream, velocity.y);
    stream_write_f(&stream, velocity.z);
    if (send_packet_except_sender(server, packet, player) == 0) {
        enet_packet_destroy(packet);
    }
}

void receive_grenade_packet(server_t* server, player_t* player, stream_t* data)
{
    uint8_t ID = stream_read_u8(data);
    if (player->id != ID) {
        LOG_WARNING("Assigned ID: %d doesnt match sent ID: %d in grenade packet", player->id, ID);
    }

    if (player->primary_fire && player->item == 1) {
        send_server_notice(player, 0, "InstaSuicideNade detected. Grenade ineffective");
        send_message_to_staff(server, 0, "Player %s (#%hhu) tried to use InstaSpadeNade", player->name, player->id);
        return;
    }

    uint64_t timeNow = get_nanos();
    if (!diff_is_older_then(timeNow, &player->timers.since_last_grenade_thrown, NANO_IN_MILLI * 500) ||
        !diff_is_older_then_dont_update(timeNow, player->timers.since_possible_spade_nade, (long) NANO_IN_MILLI * 1000))
    {
        return;
    }
    grenade_t* grenade = malloc(sizeof(grenade_t));
    if (player->grenades > 0) {
        grenade->fuse       = stream_read_f(data);
        grenade->position.x = stream_read_f(data);
        grenade->position.y = stream_read_f(data);
        grenade->position.z = stream_read_f(data);
        float velX = stream_read_f(data), velY = stream_read_f(data), velZ = stream_read_f(data);
        if (player->sprinting) {
            free(grenade);
            return;
        }
        grenade->velocity.x = velX;
        grenade->velocity.y = velY;
        grenade->velocity.z = velZ;
        vector3f_t velocity = {grenade->velocity.x - player->movement.velocity.x,
                               grenade->velocity.y - player->movement.velocity.y,
                               grenade->velocity.z - player->movement.velocity.z};
        float      length   = sqrt((velocity.x * velocity.x) + (velocity.y * velocity.y) + (velocity.z * velocity.z));
        if (length > 2) {
            float lenghtNorm = 1 / length;
            velocity.x       = ((velocity.x * lenghtNorm) * 2) + player->movement.velocity.x;
            velocity.y       = ((velocity.y * lenghtNorm) * 2) + player->movement.velocity.y;
            velocity.z       = ((velocity.z * lenghtNorm) * 2) + player->movement.velocity.z;
        }
        vector3f_t posZ1 = grenade->position;
        posZ1.z += 1;
        vector3f_t posZ2 = grenade->position;
        posZ2.z += 2;
        if (valid_pos_v3f(server, grenade->position) ||
            (valid_pos_v3f(server, posZ1) && player->movement.position.z < 0) ||
            (valid_pos_v3f(server, posZ2) && player->movement.position.z < 0))
        {
            send_grenade(server, player, grenade->fuse, grenade->position, grenade->velocity);
            grenade->sent            = 1;
            grenade->time_since_sent = get_nanos();
        }
        DL_APPEND(player->grenade, grenade);
        player->grenades--;
    } else {
        free(grenade);
    }
}
