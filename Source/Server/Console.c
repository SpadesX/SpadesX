#include <Server/Commands/Commands.h>
#include <Server/Server.h>
#include <Server/Structs/ServerStruct.h>
#include <Util/Log.h>
#include <pthread.h>
#include <readline/history.h>
#include <readline/readline.h>
#include <stdio.h>
#ifdef WIN32
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
#else
    #include <unistd.h>
#endif

volatile int ctrlc = 0;

void readline_new_line(int signal)
{
    (void) signal; // To prevent a warning about unused variable

    ctrlc = 1;
    printf("\n");
    LOG_INFO_WITHOUT_TIME("Are you sure you want to exit? (Y/n)\n");
    rl_replace_line("", 0);
    rl_on_new_line();
    rl_redisplay();
}

void* server_console(void* arg)
{
    (void) arg;
    char*     buf;
    server_t* server = (server_t*) arg;

    while ((buf = readline("\x1B[0;34mConsole> \x1B[0;37m")) != NULL) {
        if (ctrlc) { // "Are you sure?" prompt
            if (strcmp("n", buf) != 0) {
                break;
            }
            ctrlc = 0;
        } else if (strlen(buf) > 0) {
            add_history(buf);
            pthread_mutex_lock(&server_lock);
            player_t* player = NULL;
            command_handle(server, player, buf, 1);
            pthread_mutex_unlock(&server_lock);
        }
        free(buf);
    }
    rl_clear_history();
    stop_server();

#ifdef WIN32
    Sleep(5000);
#else
    sleep(5); // wait 5 seconds for the server to stop
#endif

    // if this thread is not dead at this point, then we need to stop the server by force >:)
    LOG_ERROR("Server did not respond for 5 seconds. Killing it with fire...");
    exit(-1);

    return 0;
}
