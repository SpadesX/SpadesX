#include <Server/Packets/ReceivePackets.h>
#include <Server/Server.h>
#include <Util/Checks/VectorChecks.h>
#include <Util/Physics.h>
#include <math.h>

void receive_orientation_data(server_t* server, player_t* player, stream_t* data)
{
    (void) server;

    vector3f_t orientation = {stream_read_f(data), stream_read_f(data), stream_read_f(data)};

    if ((orientation.x + orientation.y + orientation.z) == 0) {
        return;
    }

    if (!valid_vec3f(orientation)) {
        return;
    }

    float length      = sqrt((orientation.x * orientation.x) + (orientation.y * orientation.y) + (orientation.z * orientation.z));
    float norm_length = 1 / length;

    // Normalize the vectors if their length > 1
    if (length > 1.f) {
        player->movement.forward_orientation.x = orientation.x * norm_length;
        player->movement.forward_orientation.y = orientation.y * norm_length;
        player->movement.forward_orientation.z = orientation.z * norm_length;
    } else {
        player->movement.forward_orientation.x = orientation.x;
        player->movement.forward_orientation.y = orientation.y;
        player->movement.forward_orientation.z = orientation.z;
    }

    physics_reorient_player(player, &player->movement.forward_orientation);
}
