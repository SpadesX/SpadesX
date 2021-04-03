#include "Server.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main()
{
    ServerRun(DEFAULT_SERVER_PORT, 32, 2, 0, 0);
    return EXIT_SUCCESS;
}
