#ifndef MOVEMENTSTRUCT_H
#define MOVEMENTSTRUCT_H

#include <Util/Types.h>

typedef struct movement
{
    vector3f_t position;
    vector3f_t prev_position;
    vector3f_t prev_legit_pos;
    vector3f_t eye_pos;
    vector3f_t velocity;
    vector3f_t strafe_orientation;
    vector3f_t height_orientation;
    vector3f_t forward_orientation;
    vector3f_t previous_orientation;
} movement_t;

typedef struct orientation
{
    vector3f_t forward;
    vector3f_t strafe;
    vector3f_t height;
} orientation_t;

#endif
