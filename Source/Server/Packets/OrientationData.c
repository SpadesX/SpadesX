#include <Server/Packets/ReceivePackets.h>
#include <Server/Server.h>
#include <Util/Physics.h>
#include <math.h>

void receive_orientation_data(server_t* server, player_t* player, stream_t* data)
{
    (void) server;
    float x, y, z;
    x                 = stream_read_f(data);
    y                 = stream_read_f(data);
    z                 = stream_read_f(data);
    float length      = sqrt((x * x) + (y * y) + (z * z));
    float norm_legnth = 1 / length;
    // Normalize the vectors if their length > 1
    if (length > 1.f) {
        player->movement.forward_orientation.x = x * norm_legnth;
        player->movement.forward_orientation.y = y * norm_legnth;
        player->movement.forward_orientation.z = z * norm_legnth;
    } else {
        player->movement.forward_orientation.x = x;
        player->movement.forward_orientation.y = y;
        player->movement.forward_orientation.z = z;
    }

    physics_reorient_player(player, &player->movement.forward_orientation);
}
