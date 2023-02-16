#include <Server/ParseConvert.h>
#include <Server/Server.h>
#include <Util/JSONHelpers.h>
#include <Util/Notice.h>
#include <json-c/json_object.h>
#include <json-c/json_util.h>

void cmd_unban(void* p_server, command_args_t arguments)
{
    (void) p_server;
    ip_t ip;
    if (arguments.argc == 2 && parse_ip(arguments.argv[1], &ip, NULL)) {
        uint8_t             unbanned = 0;
        struct json_object* array;
        struct json_object* root = json_object_from_file("Bans.json");
        json_object_object_get_ex(root, "Bans", &array);
        int         count = json_object_array_length(array);
        const char* IPString;
        char        unbanIPString[19];
        format_ip_to_str(unbanIPString, ip);
        for (int i = 0; i < count; ++i) {
            struct json_object* objectAtIndex = json_object_array_get_idx(array, i);
            READ_STR_FROM_JSON(objectAtIndex, IPString, IP, "IP", "0.0.0.0", 1);
            if (strcmp(unbanIPString, IPString) == 0) {
                json_object_array_del_idx(array, i, 1);
                json_object_to_file("Bans.json", root);
                unbanned = 1;
            }
        }
        if (unbanned) {
            send_server_notice(arguments.player, arguments.console, "IP %s unbanned", IPString);
        } else {
            send_server_notice(arguments.player, arguments.console, "IP %s not found in banned IP list", unbanIPString);
        }
    } else {
        send_server_notice(arguments.player, arguments.console, "Incorrect amount of arguments or invalid IP");
    }
}

void cmd_unban_range(void* p_server, command_args_t arguments)
{
    (void) p_server;
    ip_t  start_range, end_range;
    char* end;
    if (arguments.argc == 2 && parse_ip(arguments.argv[1], &start_range, &end) && parse_ip(end, &end_range, NULL)) {
        uint8_t             unbanned = 0;
        struct json_object* array;
        struct json_object* root = json_object_from_file("Bans.json");
        json_object_object_get_ex(root, "Bans", &array);
        int         count = json_object_array_length(array);
        const char* start_ip_string;
        const char* end_ip_string;
        char        unban_start_range_string[19];
        char        unban_end_range_string[19];
        format_ip_to_str(unban_start_range_string, start_range);
        format_ip_to_str(unban_end_range_string, end_range);
        for (int i = 0; i < count; ++i) {
            struct json_object* object_at_index = json_object_array_get_idx(array, i);
            READ_STR_FROM_JSON(object_at_index, start_ip_string, start_of_range, "start of range", "0.0.0.0", 0);
            READ_STR_FROM_JSON(object_at_index, end_ip_string, end_of_range, "end of range", "0.0.0.0", 0);
            if (strcmp(unban_start_range_string, start_ip_string) == 0 &&
                strcmp(unban_end_range_string, end_ip_string) == 0)
            {
                json_object_array_del_idx(array, i, 1);
                json_object_to_file("Bans.json", root);
                unbanned = 1;
            }
        }
        if (unbanned) {
            send_server_notice(
            arguments.player, arguments.console, "IP range %s-%s unbanned", start_ip_string, end_ip_string);
        } else {
            send_server_notice(arguments.player,
                               arguments.console,
                               "IP range %s-%s not found in banned IP ranges",
                               unban_start_range_string,
                               unban_end_range_string);
        }
    } else {
        send_server_notice(arguments.player, arguments.console, "Incorrect amount of arguments or invalid IP");
    }
}

void cmd_undo_ban(void* p_server, command_args_t arguments)
{
    (void) p_server;
    if (arguments.argc == 1) {
        const char*         ip_string;
        const char*         start_ip;
        const char*         end_ip;
        struct json_object* array;
        struct json_object* root = json_object_from_file("Bans.json");
        json_object_object_get_ex(root, "Bans", &array);
        int                 count         = json_object_array_length(array);
        struct json_object* objectAtIndex = json_object_array_get_idx(array, count - 1);
        READ_STR_FROM_JSON(objectAtIndex, ip_string, IP, "IP", "0.0.0.0", 1);
        if (ip_string[0] == '0') {
            READ_STR_FROM_JSON(objectAtIndex, start_ip, start_of_range, "start of range", "0.0.0.0", 0);
            READ_STR_FROM_JSON(objectAtIndex, end_ip, end_of_range, "end of range", "0.0.0.0", 0);
            json_object_array_del_idx(array, count - 1, 1);
            json_object_to_file("Bans.json", root);
            send_server_notice(arguments.player, arguments.console, "IP range %s-%s unbanned", start_ip, end_ip);
        } else {
            json_object_array_del_idx(array, count - 1, 1);
            json_object_to_file("Bans.json", root);
            send_server_notice(arguments.player, arguments.console, "IP %s unbanned", ip_string);
        }
    } else {
        send_server_notice(arguments.player, arguments.console, "Too many arguments given to command");
    }
}
