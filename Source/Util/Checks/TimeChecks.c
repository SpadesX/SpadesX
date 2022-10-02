#include <Server/Structs/ServerStruct.h>

uint8_t diff_is_older_then(uint64_t timeNow, uint64_t* timeBefore, uint64_t timeDiff)
{
    if (timeNow - *timeBefore >= timeDiff) {
        *timeBefore = timeNow;
        return 1;
    } else {
        return 0;
    }
}

uint8_t diff_is_older_then_dont_update(uint64_t timeNow, uint64_t timeBefore, uint64_t timeDiff)
{
    if (timeNow - timeBefore >= timeDiff) {
        return 1;
    } else {
        return 0;
    }
}
