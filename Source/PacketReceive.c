//Copyright DarkNeutrino 2021
#include <stdio.h>
#include <enet/enet.h>
#include <libvxl/libvxl.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

#include "Structs.h"
#include "Enums.h"
#include "Types.h"
#include "DataStream.h"
#include "Queue.h"
#include "Packets.h"
#include "Map.h"
//#include "Conversion.h"

void ReceiveHitPacket(Server* server, uint8 playerID, uint8 hitPlayerID, uint8 hitType) {
	if (server->player[playerID].team != server->player[hitPlayerID].team || server->player[playerID].allowTK) {
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
	server->player[playerID].rot.x = x * length;
	server->player[playerID].rot.y = y * length;
	server->player[playerID].rot.z = z * length;
}

void ReceiveInputData(Server* server, uint8 playerID, DataStream* data)
{
	StreamSkip(data, 1); // ID
	server->player[playerID].input = ReadByte(data);
	SendInputData(server, playerID);
}

void ReceivePositionData(Server* server, uint8 playerID, DataStream* data)
{
	server->player[playerID].pos.x = ReadFloat(data);
	server->player[playerID].pos.y = ReadFloat(data);
	server->player[playerID].pos.z = ReadFloat(data);
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
			uint8 player = ReadByte(data);
			uint8 actionType = ReadByte(data);
			int X = ReadInt(data);
			int Y = ReadInt(data);
			int Z = ReadInt(data);
			switch (actionType) {
				case 0:
					libvxl_map_set(&server->map.map, X, Y, Z, color4iToInt(server->player[playerID].toolColor));
				break;
				
				case 1:
					libvxl_map_setair(&server->map.map, X, Y, Z);
				break;
				
				case 2:
					libvxl_map_setair(&server->map.map, X, Y, Z);
					libvxl_map_setair(&server->map.map, X, Y, Z-1);
					if (Z+1 < 62) {
						libvxl_map_setair(&server->map.map, X, Y, Z+1);
					}
				break;
			}
			SendBlockAction(server, playerID, actionType, X, Y, Z);
			break;
		}
		case PACKET_TYPE_BLOCK_LINE:
		{
			vec3i start;
			vec3i end;
			uint8 player = ReadByte(data);
			start.x = ReadInt(data);
			start.y = ReadInt(data);
			start.z = ReadInt(data);
			end.x = ReadInt(data);
			end.y = ReadInt(data);
			end.z = ReadInt(data);
			writeBlockLine(server, playerID, &start, &end);

			SendBlockLine(server, playerID, start, end);
			break;
		}
		case PACKET_TYPE_SET_TOOL:
		{
			uint8 player = ReadByte(data);
			uint8 tool = ReadByte(data);
			SendSetTool(server, playerID, tool);
			break;
		}
		case PACKET_TYPE_SET_COLOR:
		{
			uint8 player = ReadByte(data);
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
			uint8 player = ReadByte(data);
			uint8 wInput = ReadByte(data);
			if (server->player[playerID].weaponClip >= 0) {
				SendWeaponInput(server, playerID, wInput);
				if (wInput == 1 || wInput == 3) {
					server->player[playerID].weaponClip--;
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
			uint8 player = ReadByte(data);
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
		default:
			printf("unhandled input, id %u, code %u\n", playerID, type);
			break;
	}
}
