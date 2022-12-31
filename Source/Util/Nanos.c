#include <Server/Server.h>
#include <time.h>

uint64_t get_nanos(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return (uint64_t) ts.tv_sec * 1000000000L + ts.tv_nsec;
}
