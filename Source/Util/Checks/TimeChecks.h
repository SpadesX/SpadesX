#ifndef TIMECHECKS_H
#define TIMECHECKS_H

#include <Server/Structs/ServerStruct.h>

uint8_t diff_is_older_then(uint64_t timeNow, uint64_t* timeBefore, uint64_t timeDiff);
uint8_t diff_is_older_then_dont_update(uint64_t timeNow, uint64_t timeBefore, uint64_t timeDiff);

#endif
