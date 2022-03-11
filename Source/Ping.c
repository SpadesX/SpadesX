#include <enet/enet.h>
#include <string.h>
#include <stdio.h>

int rawUdpInterceptCallback(ENetHost* host, ENetEvent* event)
{    
    (void)event;
    if (!strncmp((char*) host->receivedData, "HELLO", host->receivedDataLength)) {
        enet_socket_send(host->socket, &host->receivedAddress, &(ENetBuffer){.data = "HI", .dataLength = 2}, 1);
        return 1;
    }
    return 0;
}
