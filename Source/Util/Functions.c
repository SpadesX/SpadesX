#include "Functions.h"

#ifdef WIN32
#include <Windows.h>
#else
#include <stdio.h>
#endif

void server_sleep(uint64 seconds)
{
#ifdef WIN32
    Sleep(seconds * 1000); // The sleep function in Windows is in milliseconds
#else
    sleep(seconds);
#endif
}
