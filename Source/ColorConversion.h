// Copyright DarkNeutrino 2021
#ifndef COLORCONVERSION_H
#define COLORCONVERSION_H

#include "Structs.h"
#include <Types.h>

int color4iToInt(Color4i color)
{
    int intColor = 0;
    intColor     = ((uint64) (((uint8) color[0]) << 24) | (uint64) (((uint8) color[1]) << 16) |
                (uint64) (((uint8) color[2]) << 8) | (uint64) ((uint8) color[3]));
    return intColor;
}

#endif
