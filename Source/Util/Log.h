#ifndef LOG_H
#define LOG_H

#include <stdio.h> // required by LOG_ERROR

void log_print_with_time(const char* format, ...);

#define LOG__INT(msg, ...)              log_print_with_time(msg "%s", __VA_ARGS__);
#define LOG__INT_WITHOUT_TIME(msg, ...) printf(msg "%s", __VA_ARGS__);
#define LOGF__INT(file, msg, ...)       fprintf(file, msg "%s", __VA_ARGS__);

#define LOG_DEBUG(...)   LOG__INT("\x1B[0;35mDEBUG: " __VA_ARGS__, "\x1B[0;37m\n")
#define LOG_INFO(...)    LOG__INT("\x1B[0;36mINFO: " __VA_ARGS__, "\x1B[0;37m\n")
#define LOG_STATUS(...)  LOG__INT("\x1B[0;32mSTATUS: " __VA_ARGS__, "\x1B[0;37m\n")
#define LOG_WARNING(...) LOG__INT("\x1B[0;33mWARNING: " __VA_ARGS__, "\x1B[0;37m\n")

#define LOG_DEBUG_WITHOUT_TIME(...)   LOG__INT_WITHOUT_TIME("\x1B[0;35mDEBUG: " __VA_ARGS__, "\x1B[0;37m\n")
#define LOG_INFO_WITHOUT_TIME(...)    LOG__INT_WITHOUT_TIME("\x1B[0;36mINFO: " __VA_ARGS__, "\x1B[0;37m\n")
#define LOG_STATUS_WITHOUT_TIME(...)  LOG__INT_WITHOUT_TIME("\x1B[0;32mSTATUS: " __VA_ARGS__, "\x1B[0;37m\n")
#define LOG_WARNING_WITHOUT_TIME(...) LOG__INT_WITHOUT_TIME("\x1B[0;33mWARNING: " __VA_ARGS__, "\x1B[0;37m\n")
#define LOG_ERROR(...)                LOGF__INT(stderr, "\x1B[0;31mERROR: " __VA_ARGS__, "\x1B[0;37m\n")

#endif // LOG_H
