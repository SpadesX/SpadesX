/*
    Copyright (c) Mathias Kaerlev 2011-2012.
    Modified by DarkNeutrino and CircumScriptor

    Cobe based upon pyspades in file world_c.cpp
    hugely modified to fit this project.
*/

#ifndef UTIL_LINE_H
#define UTIL_LINE_H

#include "../Structs.h"
/**
 * @brief Calculate block line
 *
 * @param v1 Start position
 * @param v2 End position
 * @param result Array of blocks positions
 * @return Number of block positions
 */
int line_get_blocks(const vector3i_t* v1, const vector3i_t* v2, vector3i_t* result);

#endif /* UTIL_LINE_H */
