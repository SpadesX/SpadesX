//Copyright DarkNeutrino 2021
#include <stdio.h>
#include <json-c/json.h>
#include "Types.h"
#include "Structs.h"
#include "Server.h"


int main(void) {
	struct json_object *parsed_json;
	struct json_object *portInConfig;
	struct json_object *masterInConfig;
	struct json_object *mapInConfig;

	uint16 port = DEFAULT_SERVER_PORT;
	uint8 master = 1;
	
	parsed_json = json_object_from_file("config.json");
	if (json_object_object_get_ex(parsed_json, "port", &portInConfig) == 0) {
		printf("Failed to find port number in config\n");
		return -1;
	}
	if (json_object_object_get_ex(parsed_json, "master", &masterInConfig) == 0) {
		printf("Failed to find master variable in config\n");
		return -1;
	}
	if (json_object_object_get_ex(parsed_json, "map", &mapInConfig) == 0) {
		printf("Failed to find map name in config\n");
		return -1;
	}
	const char* map = json_object_get_string(mapInConfig);
	port = json_object_get_int(portInConfig);
	master = json_object_get_int(masterInConfig);

	StartServer(port, 64, 2, 0, 0, master, map);
	return 0;
}
