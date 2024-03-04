#ifndef CONSOLE_H
#define CONSOLE_H

void* server_console(void* arg);
void  handle_sigint(); // Old signature: readline_new_line(int signal)

#endif
