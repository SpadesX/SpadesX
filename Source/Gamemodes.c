#include "Protocol.h"
#include "Structs.h"

#include <math.h>
#include <string.h>

void initGameMode(Server* server, uint8 gamemode)
{
    if (gamemode == 0) {
        server->protocol.mode = GAME_MODE_CTF;
    } else if (gamemode == 1) {
        server->protocol.mode = GAME_MODE_TC;
    }

    if (server->protocol.mode == GAME_MODE_CTF) {
        memcpy(server->gamemodeName, "ctf", strlen("ctf") + 1);
        // Init CTF
        server->protocol.ctf.score[0]   = 0;
        server->protocol.ctf.score[1]   = 0;
        server->protocol.ctf.scoreLimit = 10;
        server->protocol.ctf.intelFlags = 0;
        // intel
        server->protocol.ctf.intel[0]     = SetIntelTentSpawnPoint(server, 0);
        server->protocol.ctf.intel[1]     = SetIntelTentSpawnPoint(server, 1);
        server->protocol.ctf.intelHeld[0] = 0;
        server->protocol.ctf.intelHeld[1] = 0;
        // bases
        server->protocol.ctf.base[0] = SetIntelTentSpawnPoint(server, 0);
        server->protocol.ctf.base[1] = SetIntelTentSpawnPoint(server, 1);

        server->protocol.ctf.base[0].x = floorf(server->protocol.ctf.base[0].x);
        server->protocol.ctf.base[0].y = floorf(server->protocol.ctf.base[0].y);
        server->protocol.ctf.base[1].x = floorf(server->protocol.ctf.base[1].x);
        server->protocol.ctf.base[1].y = floorf(server->protocol.ctf.base[1].y);
    } else if (server->protocol.mode == GAME_MODE_TC) {
        memcpy(server->gamemodeName, "tc", strlen("tc") + 1);
    } else {
        memcpy(server->gamemodeName, "IDK", strlen("IDK") + 1);
    }
}
