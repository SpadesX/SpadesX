#include "Server.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main()
{
    Server server;
    ServerStart(&server, DEFAULT_SERVER_PORT, 32, 12, 0, 0);

    printf("INFO: starting loop\n");

    int stopRunning = 0;
    while (!stopRunning) {
        ServerStep(&server, 1);
    }

    printf("INFO: loop end\n");

    ServerDestroy(&server);
    return EXIT_SUCCESS;
}
