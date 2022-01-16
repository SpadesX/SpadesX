#include "Protocol.h"
#include "Structs.h"

#include <math.h>
#include <string.h>

void initGameMode(Server* server, uint8 gamemode)
{
    if (gamemode == 0) {
        server->protocol.currentGameMode = GAME_MODE_CTF;
    } else if (gamemode == 1) {
        server->protocol.currentGameMode = GAME_MODE_TC;
    }

    if (server->protocol.currentGameMode == GAME_MODE_CTF) {
        memcpy(server->gamemodeName, "ctf", strlen("ctf") + 1);
        // Init CTF
        server->protocol.gameMode.score[0]   = 0;
        server->protocol.gameMode.score[1]   = 0;
        server->protocol.gameMode.scoreLimit = 10;
        server->protocol.gameMode.intelFlags = 0;
        // intel
        server->protocol.gameMode.intel[0]     = SetIntelTentSpawnPoint(server, 0);
        server->protocol.gameMode.intel[1]     = SetIntelTentSpawnPoint(server, 1);
        server->protocol.gameMode.intelHeld[0] = 0;
        server->protocol.gameMode.intelHeld[1] = 0;
        // bases
        server->protocol.gameMode.base[0] = SetIntelTentSpawnPoint(server, 0);
        server->protocol.gameMode.base[1] = SetIntelTentSpawnPoint(server, 1);

        server->protocol.gameMode.base[0].x = floorf(server->protocol.gameMode.base[0].x);
        server->protocol.gameMode.base[0].y = floorf(server->protocol.gameMode.base[0].y);
        server->protocol.gameMode.base[1].x = floorf(server->protocol.gameMode.base[1].x);
        server->protocol.gameMode.base[1].y = floorf(server->protocol.gameMode.base[1].y);
    } else if (server->protocol.currentGameMode == GAME_MODE_TC) {
        memcpy(server->gamemodeName, "tc", strlen("tc") + 1);
    }
    else if (server->protocol.currentGameMode == GAME_MODE_BABEL) {
        memcpy(server->gamemodeName, "babel", strlen("babel") + 1);
    }
    else if (server->protocol.currentGameMode == GAME_MODE_ARENA) {
        memcpy(server->gamemodeName, "arena", strlen("arena") + 1);
    } else {
        memcpy(server->gamemodeName, "IDK", strlen("IDK") + 1);
    }
}
