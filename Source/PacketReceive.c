//Copyright DarkNeutrino 2021
#include <stdio.h>
#include <enet/enet.h>
#include <libvxl/libvxl.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

#include "Protocol.h"
#include "Structs.h"
#include "Enums.h"
#include "Types.h"
#include "DataStream.h"
#include "Queue.h"
#include "Packets.h"
#include "Map.h"
#include "ColorConversion.h"

void reorient_player(Server* server, uint8 playerID, Vector3f* orientation);

static unsigned long long get_nanos(void) {
    struct timespec ts;
    timespec_get(&ts, TIME_UTC);
    return (unsigned long long)ts.tv_sec * 1000000000L + ts.tv_nsec;
}

void ReceiveGrenadePacket(Server *server, uint8 playerID, DataStream *data) {
	for (int i = 0; i < 3; ++i) {
		if (server->player[playerID].grenade[i].sent == 0) {
			server->player[playerID].grenade[i].fuse = ReadFloat(data);
			server->player[playerID].grenade[i].position.x = ReadFloat(data);
			server->player[playerID].grenade[i].position.y = ReadFloat(data);
			server->player[playerID].grenade[i].position.z = ReadFloat(data);
			server->player[playerID].grenade[i].velocity.x = ReadFloat(data);
			server->player[playerID].grenade[i].velocity.y = ReadFloat(data);
			server->player[playerID].grenade[i].velocity.z = ReadFloat(data);
			SendGrenade(server, playerID, server->player[playerID].grenade[i].fuse, server->player[playerID].grenade[i].position, server->player[playerID].grenade[i].velocity);
			server->player[playerID].grenade[i].sent = 1;
			server->player[playerID].grenade[i].timeSinceSent = get_nanos();
			break;
		}
	}
}

void ReceiveHitPacket(Server* server, uint8 playerID, uint8 hitPlayerID, uint8 hitType) {
	if ((server->player[playerID].team != server->player[hitPlayerID].team || server->player[playerID].allowTeamKilling) && (server->player[playerID].allowKilling && server->globalAK)) {
			switch (server->player[playerID].weapon) {
				case WEAPON_RIFLE:
				{
					switch (hitType) {
						case HIT_TYPE_HEAD:
						{
							sendKillPacket(server, playerID, hitPlayerID, 1, 5);
							break;
						}
						case HIT_TYPE_TORSO:
						{
							sendHP(server, playerID, hitPlayerID, 49, 1, 0, 5);
							break;
						}
						case HIT_TYPE_ARMS:
						{
							sendHP(server, playerID, hitPlayerID, 33, 1, 0, 5);
							break;
						}
						case HIT_TYPE_LEGS:
						{
							sendHP(server, playerID, hitPlayerID, 33, 1, 0, 5);
							break;
						}
					}
					break;
				}
				case WEAPON_SMG:
				{
					switch (hitType) {
						case HIT_TYPE_HEAD:
						{
							sendHP(server, playerID, hitPlayerID, 75, 1, 1, 5);
							break;
						}
						case HIT_TYPE_TORSO:
						{
							sendHP(server, playerID, hitPlayerID, 29, 1, 0, 5);
							break;
						}
						case HIT_TYPE_ARMS:
						{
							sendHP(server, playerID, hitPlayerID, 18, 1, 0, 5);
							break;
						}
						case HIT_TYPE_LEGS:
						{
							sendHP(server, playerID, hitPlayerID, 18, 1, 0, 5);
							break;
						}
					}
					break;
				}
				case WEAPON_SHOTGUN:
				{
					switch (hitType) {
						case HIT_TYPE_HEAD:
						{
							sendHP(server, playerID, hitPlayerID, 37, 1, 1, 5);
							break;
						}
						case HIT_TYPE_TORSO:
						{
							sendHP(server, playerID, hitPlayerID, 27, 1, 0, 5);
							break;
						}
						case HIT_TYPE_ARMS:
						{
							sendHP(server, playerID, hitPlayerID, 16, 1, 0, 5);
							break;
						}
						case HIT_TYPE_LEGS:
						{
							sendHP(server, playerID, hitPlayerID, 16, 1, 0, 5);
							break;
						}
					}
					break;
				}
			}
			if (hitType == HIT_TYPE_MELEE) {
				sendHP(server, playerID, hitPlayerID, 80, 1, 2, 5);
			}
			}
}

void ReceiveOrientationData(Server* server, uint8 playerID, DataStream* data)
{
	float x, y, z;
	x = ReadFloat(data);
	y = ReadFloat(data);
	z = ReadFloat(data);
	float length = 1/sqrt((x*x) + (y*y) + (z*z));
	//Normalize the vectors as soon as we get them
	server->player[playerID].movement.forwardOrientation.x = x * length;
	server->player[playerID].movement.forwardOrientation.y = y * length;
	server->player[playerID].movement.forwardOrientation.z = z * length;
	reorient_player(server, playerID, &server->player[playerID].movement.forwardOrientation);
}

void ReceiveInputData(Server* server, uint8 playerID, DataStream* data)
{
	bool bits[8];
	uint8 mask = 1;
	StreamSkip(data, 1); // ID
	server->player[playerID].input = ReadByte(data);
	for (int i = 0; i < 8; i++) {
		bits[i] = (server->player[playerID].input >> i) & mask;
	}
	server->player[playerID].movForward = bits[0];
	server->player[playerID].movBackwards = bits[1];
	server->player[playerID].movLeft = bits[2];
	server->player[playerID].movRight = bits[3];
	server->player[playerID].jumping = bits[4];
	server->player[playerID].crouching = bits[5];
	server->player[playerID].sneaking = bits[6];
	server->player[playerID].sprinting = bits[7];
	SendInputData(server, playerID);
}

void ReceivePositionData(Server* server, uint8 playerID, DataStream* data)
{
	server->player[playerID].movement.position.x = ReadFloat(data);
	server->player[playerID].movement.position.y = ReadFloat(data);
	server->player[playerID].movement.position.z = ReadFloat(data);
}

void ReceiveExistingPlayer(Server* server, uint8 playerID, DataStream* data)
{
	uint8 id = ReadByte(data);
	if (playerID != id) {
		WARNING("different player id");
	}

	server->player[playerID].team   = ReadByte(data);
	server->player[playerID].weapon = ReadByte(data);
	server->player[playerID].item   = ReadByte(data);
	server->player[playerID].kills  = ReadInt(data);

	ReadColor3i(data, server->player[playerID].color);
	server->player[playerID].ups = 60;

	uint32 length = DataLeft(data);
	if (length > 16) {
		WARNING("name too long");
	} else {
		server->player[playerID].name[length] = '\0';
		ReadArray(data, server->player[playerID].name, length);
		int count = 0;
		for (uint8 i = 0; i < server->protocol.maxPlayers; i++) {
			if (server->player[i].state != STATE_DISCONNECTED && i != playerID) {
				if (strcmp(server->player[playerID].name, server->player[i].name) == 0) {
					count++;
				}
			}
		}
		if (count > 0) {
			char idChar[3];
			sprintf(idChar, "%d", playerID);
			strcat(server->player[playerID].name, idChar);
		}
	}
	switch (server->player[playerID].weapon) {
		case 0:
			server->player[playerID].weaponReserve = 50;
			server->player[playerID].weaponClip = 10;
			break;
		case 1:
			server->player[playerID].weaponReserve = 120;
			server->player[playerID].weaponClip = 30;
			break;
		case 2:
			server->player[playerID].weaponReserve = 48;
			server->player[playerID].weaponClip = 6;
			break;
	}
	server->player[playerID].state = STATE_SPAWNING;
}

void OnPacketReceived(Server* server, uint8 playerID, DataStream* data, ENetEvent event)
{
	PacketID type = (PacketID) ReadByte(data);
	switch (type) {
		case PACKET_TYPE_EXISTING_PLAYER:
			ReceiveExistingPlayer(server, playerID, data);
			break;
		case PACKET_TYPE_POSITION_DATA:
			ReceivePositionData(server, playerID, data);
			break;
		case PACKET_TYPE_ORIENTATION_DATA:
			ReceiveOrientationData(server, playerID, data);
			break;
		case PACKET_TYPE_INPUT_DATA:
			ReceiveInputData(server, playerID, data);
			break;
		case PACKET_TYPE_CHAT_MESSAGE:
			sendMessage(event, data, server);
			break;
		case PACKET_TYPE_BLOCK_ACTION:
		{
			if (server->player[playerID].canBuild && server->globalAB) {
			ReadByte(data);
			uint8 actionType = ReadByte(data);
			int X = ReadInt(data);
			int Y = ReadInt(data);
			int Z = ReadInt(data);
			Vector3f vectorBlock = {X, Y, Z};
			Vector3f playerVector = server->player[playerID].movement.position;
			printf("%d %f %f\n", DistanceIn3D(vectorBlock, playerVector), vectorBlock.z, playerVector.z);
			if (((server->player[playerID].blocks > 0) && (server->player[playerID].item == 0) && (actionType == 1 || actionType == 2)) || (server->player[playerID].item == 1 && actionType == 0) || (server->player[playerID].item == 2 && actionType == 1)) {
				if (DistanceIn3D(vectorBlock, playerVector) <= 4) {
					switch (actionType) {
						case 0:
								libvxl_map_set(&server->map.map, X, Y, Z, color4iToInt(server->player[playerID].toolColor));
								server->player[playerID].blocks--;
								moveIntelAndTentUp(server);
						break;

						case 1:
							libvxl_map_setair(&server->map.map, X, Y, Z);
							server->player[playerID].blocks++;
						break;

						case 2:
							for (int z = Z -1; z <= Z + 1; z++) {
								if (z < 62) {
									libvxl_map_setair(&server->map.map, X, Y, z);
									server->player[playerID].blocks++;
								}
							}
						break;
					}
						SendBlockAction(server, playerID, actionType, X, Y, Z);
						moveIntelAndTentDown(server);
				}
			}
			else {
				printf("Player: #%d may be using BlockExploit with Item: %d and Action: %d\n", playerID, server->player[playerID].item, actionType);
			}
			}
			break;
		}
		case PACKET_TYPE_BLOCK_LINE:
		{
			if (server->player[playerID].blocks > 0 && server->player[playerID].canBuild && server->globalAB && server->player[playerID].item == 1) {
			vec3i start;
			vec3i end;
			ReadByte(data);
			start.x = ReadInt(data);
			start.y = ReadInt(data);
			start.z = ReadInt(data);
			end.x = ReadInt(data);
			end.y = ReadInt(data);
			end.z = ReadInt(data);
			Vector3f startF = {start.x, start.y, start.z};
			Vector3f endF = {end.x, end.y, end.z};
			if (DistanceIn3D(endF, server->player[playerID].movement.position) <= 4 && DistanceIn3D(startF, server->player[playerID].locAtClick) <= 4) {
				writeBlockLine(server, playerID, &start, &end);
				moveIntelAndTentUp(server);
				SendBlockLine(server, playerID, start, end);
			}
			}
			break;
		}
		case PACKET_TYPE_SET_TOOL:
		{
			ReadByte(data);
			uint8 tool = ReadByte(data);
			server->player[playerID].item = tool;
			SendSetTool(server, playerID, tool);
			break;
		}
		case PACKET_TYPE_SET_COLOR:
		{
			ReadByte(data);
			uint8 B = ReadByte(data);
			uint8 G = ReadByte(data);
			uint8 R = ReadByte(data);
			uint8 A = 0;

			server->player[playerID].toolColor[0] = A;
			server->player[playerID].toolColor[1] = R;
			server->player[playerID].toolColor[2] = G;
			server->player[playerID].toolColor[3] = B;
			SendSetColor(server, playerID, R, G, B);
			break;
		}
		
		case PACKET_TYPE_WEAPON_INPUT:
		{
			uint8 mask = 1;
			uint8 bits[8];
			ReadByte(data);
			uint8 wInput = ReadByte(data);
			if (server->player[playerID].weaponClip >= 0) {
				SendWeaponInput(server, playerID, wInput);
				for (int i = 0; i < 8; i++) {
					bits[i] = (wInput >> i) & mask;
				}
				server->player[playerID].primary_fire = bits[0];
				server->player[playerID].secondary_fire = bits[1];
				if (server->player[playerID].primary_fire) {
					server->player[playerID].weaponClip--;
				}
				if (server->player[playerID].secondary_fire && server->player[playerID].item == 1) {
					server->player[playerID].locAtClick = server->player[playerID].movement.position;
				}
			}
			else {
				//sendKillPacket(server, playerID, playerID, 0, 30);
			}
			break;
		}
		
		case PACKET_TYPE_HIT_PACKET:
		{
			uint8 hitPlayerID = ReadByte(data);
			Hit hitType = ReadByte(data);
			ReceiveHitPacket(server, playerID, hitPlayerID, hitType);
			break;
		}
		case PACKET_TYPE_WEAPON_RELOAD:
		{
			ReadByte(data);
			uint8 reserve = ReadByte(data);
			uint8 clip = ReadByte(data);
			server->player[playerID].weaponReserve = 50; //Temporary
			server->player[playerID].weaponClip = 10;
			SendWeaponReload(server, playerID);
			break;
		}
		case PACKET_TYPE_CHANGE_TEAM:
		{
			uint8 player = ReadByte(data);
			server->player[player].team = ReadByte(data);
			sendKillPacket(server, playerID, playerID, 5, 5);
			server->player[playerID].state = STATE_WAITING_FOR_RESPAWN;
			break;
		}
		case PACKET_TYPE_CHANGE_WEAPON:
		{
			uint8 player = ReadByte(data);
			server->player[player].weapon = ReadByte(data);
			sendKillPacket(server, playerID, playerID, 6, 5);
			server->player[playerID].state = STATE_WAITING_FOR_RESPAWN;
			break;
		}
		case PACKET_TYPE_GRENADE_PACKET:
		{
			if (server->player[playerID].grenades > 0) {
				ReadByte(data);
				ReceiveGrenadePacket(server, playerID, data);
				server->player[playerID].grenades--;
			}
			break;
		}
		default:
			printf("unhandled input, id %u, code %u\n", playerID, type);
			break;
	}
}
