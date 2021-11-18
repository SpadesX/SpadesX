// Copyright DarkNeutrino 2021
#ifndef ENUMS_H
#define ENUMS_H

#include <Types.h>

typedef enum {
    STATE_DISCONNECTED,
    STATE_STARTING_MAP,
    STATE_LOADING_CHUNKS,
    STATE_JOINING,
    STATE_PICK_SCREEN,
    STATE_SPAWNING,
    STATE_WAITING_FOR_RESPAWN,
    STATE_READY,
} State;

typedef enum {
    VERSION_0_75 = 3,
    VERSION_0_76 = 4,
} ProtocolVersion;

typedef enum {
    REASON_BANNED                 = 1,
    REASON_IP_LIMIT_EXCEEDED      = 2,
    REASON_WRONG_PROTOCOL_VERSION = 3,
    REASON_SERVER_FULL            = 4,
    REASON_KICKED                 = 10,
} DisconnectedReason;

typedef enum {
    PACKET_TYPE_POSITION_DATA     = 0,
    PACKET_TYPE_ORIENTATION_DATA  = 1,
    PACKET_TYPE_WORLD_UPDATE      = 2,
    PACKET_TYPE_INPUT_DATA        = 3,
    PACKET_TYPE_WEAPON_INPUT      = 4,
    PACKET_TYPE_HIT_PACKET        = 5, // client
    PACKET_TYPE_SET_HP            = 5, // server
    PACKET_TYPE_GRENADE_PACKET    = 6,
    PACKET_TYPE_SET_TOOL          = 7,
    PACKET_TYPE_SET_COLOR         = 8,
    PACKET_TYPE_EXISTING_PLAYER   = 9,
    PACKET_TYPE_SHORT_PLAYER_DATA = 10,
    PACKET_TYPE_MOVE_OBJECT       = 11,
    PACKET_TYPE_CREATE_PLAYER     = 12,
    PACKET_TYPE_BLOCK_ACTION      = 13,
    PACKET_TYPE_BLOCK_LINE        = 14,
    PACKET_TYPE_STATE_DATA        = 15,
    PACKET_TYPE_KILL_ACTION       = 16,
    PACKET_TYPE_CHAT_MESSAGE      = 17,
    PACKET_TYPE_MAP_START         = 18,
    PACKET_TYPE_MAP_CHUNK         = 19,
    PACKET_TYPE_PLAYER_LEFT       = 20,
    PACKET_TYPE_TERRITORY_CAPTURE = 21,
    PACKET_TYPE_PROGRESS_BAR      = 22,
    PACKET_TYPE_INTEL_CAPTURE     = 23,
    PACKET_TYPE_INTEL_PICKUP      = 24,
    PACKET_TYPE_INTEL_DROP        = 25,
    PACKET_TYPE_RESTOCK           = 26,
    PACKET_TYPE_FOG_COLOR         = 27,
    PACKET_TYPE_WEAPON_RELOAD     = 28,
    PACKET_TYPE_CHANGE_TEAM       = 29,
    PACKET_TYPE_CHANGE_WEAPON     = 30,
    PACKET_TYPE_MAP_CACHED        = 31,
    PACKET_TYPE_VERSION_REQUEST   = 33,
    PACKET_TYPE_VERSION_RESPONSE  = 34,
} PacketID;

typedef enum {
    GAME_MODE_CTF = 0,
    GAME_MODE_TC  = 1,
} GameMode;

typedef enum {
    TEAM_SPECTATOR = (uint8) -1,
    TEAM_A         = 0,
    TEAM_B         = 1,
} Team;

typedef enum {
    WEAPON_RIFLE   = 0,
    WEAPON_SMG     = 1,
    WEAPON_SHOTGUN = 2,
} Weapon;

typedef enum {
    INTEL_TEAM_A = 1,
    INTEL_TEAM_B = 2,
} IntelFlag;

typedef enum {
    TOOL_SPADE   = 0,
    TOOL_BLOCK   = 1,
    TOOL_GUN     = 2,
    TOOL_GRENADE = 3,
} Tool;

typedef enum {
    HIT_TYPE_TORSO = 0,
    HIT_TYPE_HEAD  = 1,
    HIT_TYPE_ARMS  = 2,
    HIT_TYPE_LEGS  = 3,
    HIT_TYPE_MELEE = 4,
} Hit;

typedef enum {
    RIFLE_DEFAULT_CLIP = 10,
    SMG_DEFAULT_CLIP = 30,
    SHOTGUN_DEFAULT_CLIP = 6,
} WeaponDefaultClip;

typedef enum {
    RIFLE_DEFAULT_RESERVE = 50,
    SMG_DEFAULT_RESERVE = 120,
    SHOTGUN_DEFAULT_RESERVE = 48,
} WeaponDefaultReserve;

#endif /* ENUMS_H */
