#include <Server/Server.h>
#include <Util/Checks/PositionChecks.h>

void send_position_packet(server_t* server, uint8_t player_id, float x, float y, float z)
{
    if (server->protocol.num_players == 0) {
        return;
    }
    ENetPacket* packet = enet_packet_create(NULL, 13, ENET_PACKET_FLAG_RELIABLE);
    stream_t    stream = {packet->data, packet->dataLength, 0};
    player_t*   player = &server->player[player_id];
    stream_write_u8(&stream, PACKET_TYPE_POSITION_DATA);
    stream_write_f(&stream, x);
    stream_write_f(&stream, y);
    stream_write_f(&stream, z);
    if (enet_peer_send(player->peer, 0, packet) != 0) {
        enet_packet_destroy(packet);
    }
}

void receive_position_data(server_t* server, uint8_t player_id, stream_t* data)
{
    float x, y, z;
    x = stream_read_f(data);
    y = stream_read_f(data);
    z = stream_read_f(data);

    player_t* player = &server->player[player_id];
#ifdef DEBUG
    LOG_DEBUG("Player: %d, Our X: %f, Y: %f, Z: %f Actual X: %f, Y: %f, Z: %f",
              player_id,
              player->movement.position.x,
              player->movement.position.y,
              player->movement.position.z,
              x,
              y,
              z);
    LOG_DEBUG("Player: %d, Z solid: %d, Z+1 solid: %d, Z+2 solid: %d, Z: %d, Z+1: %d, Z+2: %d, Crouching: %d",
              player_id,
              mapvxl_is_solid(&server->s_map.map, (int) x, (int) y, (int) z),
              mapvxl_is_solid(&server->s_map.map, (int) x, (int) y, (int) z + 1),
              mapvxl_is_solid(&server->s_map.map, (int) x, (int) y, (int) z + 2),
              (int) z,
              (int) z + 1,
              (int) z + 2,
              player->crouching);
#endif
    player->movement.prev_position = player->movement.position;
    player->movement.position.x    = x;
    player->movement.position.y    = y;
    player->movement.position.z    = z;
    /*uint8_t resetTime                                = 1;
    if (!diff_is_older_then_dont_update(
        getNanos(), player->timers.duringNoclipPeriod, (uint64_t) NANO_IN_SECOND * 10))
    {
        resetTime = 0;
        if (validPlayerPos(server, player_id, x, y, z) == 0) {
            player->invalidPosCount++;
            if (player->invalidPosCount == 5) {
                sendMessageToStaff(server, "Player %s (#%hhu) is most likely trying to avoid noclip detection");
            }
        }
    }
    if (resetTime) {
        player->timers.duringNoclipPeriod = getNanos();
        player->invalidPosCount           = 0;
    }*/

    if (valid_pos_3f(server, player_id, x, y, z)) {
        player->movement.prev_legit_pos = player->movement.position;
    } /*else if (validPlayerPos(server,
                              player_id,
                              player->movement.position.x,
                              player->movement.position.y,
                              player->movement.position.z) == 0 &&
               validPlayerPos(server,
                              player_id,
                              player->movement.prevPosition.x,
                              player->movement.prevPosition.y,
                              player->movement.prevPosition.z) == 0)
    {
        LOG_WARNING("Player %s (#%hhu) may be noclipping!", player->name, player_id);
        sendMessageToStaff(server, "Player %s (#%d) may be noclipping!", player->name, player_id);
        if (validPlayerPos(server,
                           player_id,
                           player->movement.prevLegitPos.x,
                           player->movement.prevLegitPos.y,
                           player->movement.prevLegitPos.z) == 0)
        {
            if (getPlayerUnstuck(server, player_id) == 0) {
                SetPlayerRespawnPoint(server, player_id);
                sendServerNotice(
                server, player_id, 0, "Server could not find a free position to get you unstuck. Reseting to spawn");
                LOG_WARNING(
                "Could not find legit position for player %s (#%d) to get them unstuck. Resetting to spawn. "
                "Go check them!",
                player->name,
                player_id);
                sendMessageToStaff(
                server,
                "Could not find legit position for player %s (#%d) to get them unstuck. Resetting to spawn. "
                "Go check them!",
                player->name,
                player_id);
                player->movement.prevLegitPos = player->movement.position;
            }
        }
        SendPositionPacket(server,
                           player_id,
                           player->movement.prevLegitPos.x,
                           player->movement.prevLegitPos.y,
                           player->movement.prevLegitPos.z);
    }*/
}
