#include <math.h>
#include <stddef.h>

// SpadesX
#include "../../Extern/libmapvxl/libmapvxl.h"
#include "../Structs.h"
#include "Types.h"
#include "Physics.h"

#define SQRT                 0.70710678f
#define MINERANGE            3
#define FALL_SLOW_DOWN       0.24f
#define FALL_DAMAGE_VELOCITY 0.58f
#define FALL_DAMAGE_SCALAR   4096

// globals to make porting easier
float ftotclk;
float fsynctics;

static inline void getOrientation(Orientation* o, float orientation_x, float orientation_y, float orientation_z)
{
    float f;
    o->forward.x = orientation_x;
    o->forward.y = orientation_y;
    o->forward.z = orientation_z;
    f            = sqrtf(orientation_x * orientation_x + orientation_y * orientation_y);
    o->strafe.x  = -orientation_y / f;
    o->strafe.y  = orientation_x / f;
    o->strafe.z  = 0.0f;
    o->height.x  = -orientation_z * o->strafe.y;
    o->height.y  = orientation_z * o->strafe.x;
    o->height.z  = orientation_x * o->strafe.y - orientation_y * o->strafe.x;
}

float distance3d(float x1, float y1, float z1, float x2, float y2, float z2)
{
    return sqrtf(pow(x2 - x1, 2) + pow(y2 - y1, 2) + pow(z2 - z1, 2));
}

int validateHit(Vector3f shooter, Vector3f orientation, Vector3f otherPos, float tolerance)
{
    float       cx, cy, cz, r, x, y;
    Orientation o;
    getOrientation(&o, orientation.x, orientation.y, orientation.z);
    otherPos.x -= shooter.x;
    otherPos.y -= shooter.y;
    otherPos.z -= shooter.z;
    cz = otherPos.x * o.forward.x + otherPos.y * o.forward.y + otherPos.z * o.forward.z;
    r  = 1.f / cz;
    cx = otherPos.x * o.strafe.x + otherPos.y * o.strafe.y + otherPos.z * o.strafe.z;
    x  = cx * r;
    cy = otherPos.x * o.height.x + otherPos.y * o.height.y + otherPos.z * o.height.z;
    y  = cy * r;
    r *= tolerance;
    int ret = (x - r < 0 && x + r > 0 && y - r < 0 && y + r > 0);
    return ret;
}

// silly VOXLAP function
static inline void ftol(float f, long* a)
{
    *a = (long) f;
}

// same as isvoxelsolid but water is empty && out of bounds returns true
static inline int clipbox(Server* server, float x, float y, float z)
{
    int sz;

    if (x < 0 || x >= server->map.map.MAP_X_MAX || y < 0 || y >= server->map.map.MAP_Y_MAX)
        return 1;
    else if (z < 0)
        return 0;
    sz = (int) z;
    if (sz == server->map.map.MAP_Z_MAX - 1)
        sz = server->map.map.MAP_Z_MAX - 2;
    else if (sz >= server->map.map.MAP_Z_MAX)
        return 1;
    return mapvxlIsSolid(&server->map.map, (int) x, (int) y, sz);
}

// same as isvoxelsolid() but with wrapping
static inline long isvoxelsolidwrap(Server* server, long x, long y, long z)
{
    if (z < 0)
        return 0;
    else if (z >= server->map.map.MAP_Z_MAX)
        return 1;
    return mapvxlIsSolid(&server->map.map, (int) x & (server->map.map.MAP_X_MAX-1), (int) y & (server->map.map.MAP_Y_MAX-1), z);
}

// same as isvoxelsolid but water is empty
static inline long clipworld(Server* server, long x, long y, long z)
{
    int sz;
    if (x < 0 || x >= server->map.map.MAP_X_MAX || y < 0 || y >= server->map.map.MAP_Y_MAX)
        return 0;
    if (z < 0)
        return 0;
    sz = (int) z;
    if (sz == server->map.map.MAP_Z_MAX - 1)
        sz = server->map.map.MAP_Z_MAX - 2;
    else if (sz >= server->map.map.MAP_Z_MAX - 1)
        return 1;
    else if (sz < 0)
        return 0;
    return mapvxlIsSolid(&server->map.map, (int) x, (int) y, sz);
}

long canSee(Server* server, float x0, float y0, float z0, float x1, float y1, float z1)
{
    Vector3f f, g;
    Vector3l a, c, d, p, i;
    d.x      = 0;
    d.y      = 0;
    d.z      = 0;
    long cnt = 0;

    ftol(x0 - .5f, &a.x);
    ftol(y0 - .5f, &a.y);
    ftol(z0 - .5f, &a.z);
    ftol(x1 - .5f, &c.x);
    ftol(y1 - .5f, &c.y);
    ftol(z1 - .5f, &c.z);

    if (c.x < a.x) {
        d.x = -1;
        f.x = x0 - a.x;
        g.x = (x0 - x1) * 1024;
        cnt += a.x - c.x;
    } else if (c.x != a.x) {
        d.x = 1;
        f.x = a.x + 1 - x0;
        g.x = (x1 - x0) * 1024;
        cnt += c.x - a.x;
    } else
        f.x = g.x = 0;
    if (c.y < a.y) {
        d.y = -1;
        f.y = y0 - a.y;
        g.y = (y0 - y1) * 1024;
        cnt += a.y - c.y;
    } else if (c.y != a.y) {
        d.y = 1;
        f.y = a.y + 1 - y0;
        g.y = (y1 - y0) * 1024;
        cnt += c.y - a.y;
    } else
        f.y = g.y = 0;
    if (c.z < a.z) {
        d.z = -1;
        f.z = z0 - a.z;
        g.z = (z0 - z1) * 1024;
        cnt += a.z - c.z;
    } else if (c.z != a.z) {
        d.z = 1;
        f.z = a.z + 1 - z0;
        g.z = (z1 - z0) * 1024;
        cnt += c.z - a.z;
    } else
        f.z = g.z = 0;

    ftol(f.x * g.z - f.z * g.x, &p.x);
    ftol(g.x, &i.x);
    ftol(f.y * g.z - f.z * g.y, &p.y);
    ftol(g.y, &i.y);
    ftol(f.y * g.x - f.x * g.y, &p.z);
    ftol(g.z, &i.z);

    if (cnt > 32)
        cnt = 32;
    while (cnt) {
        if (((p.x | p.y) >= 0) && (a.z != c.z)) {
            a.z += d.z;
            p.x -= i.x;
            p.y -= i.y;
        } else if ((p.z >= 0) && (a.x != c.x)) {
            a.x += d.x;
            p.x += i.z;
            p.z -= i.y;
        } else {
            a.y += d.y;
            p.y += i.z;
            p.z += i.x;
        }

        if (isvoxelsolidwrap(server, a.x, a.y, a.z))
            return 0;
        cnt--;
    }
    return 1;
}

long castRay(Server* server,
             float   x0,
             float   y0,
             float   z0,
             float   x1,
             float   y1,
             float   z1,
             float   length,
             long*   x,
             long*   y,
             long*   z)
{
    x1 = x0 + x1 * length;
    y1 = y0 + y1 * length;
    z1 = z0 + z1 * length;
    Vector3f f, g;
    Vector3l a, c, d, p, i;
    d.x      = 0;
    d.y      = 0;
    d.z      = 0;
    long cnt = 0;

    ftol(x0 - .5f, &a.x);
    ftol(y0 - .5f, &a.y);
    ftol(z0 - .5f, &a.z);
    ftol(x1 - .5f, &c.x);
    ftol(y1 - .5f, &c.y);
    ftol(z1 - .5f, &c.z);

    if (c.x < a.x) {
        d.x = -1;
        f.x = x0 - a.x;
        g.x = (x0 - x1) * 1024;
        cnt += a.x - c.x;
    } else if (c.x != a.x) {
        d.x = 1;
        f.x = a.x + 1 - x0;
        g.x = (x1 - x0) * 1024;
        cnt += c.x - a.x;
    } else
        f.x = g.x = 0;
    if (c.y < a.y) {
        d.y = -1;
        f.y = y0 - a.y;
        g.y = (y0 - y1) * 1024;
        cnt += a.y - c.y;
    } else if (c.y != a.y) {
        d.y = 1;
        f.y = a.y + 1 - y0;
        g.y = (y1 - y0) * 1024;
        cnt += c.y - a.y;
    } else
        f.y = g.y = 0;
    if (c.z < a.z) {
        d.z = -1;
        f.z = z0 - a.z;
        g.z = (z0 - z1) * 1024;
        cnt += a.z - c.z;
    } else if (c.z != a.z) {
        d.z = 1;
        f.z = a.z + 1 - z0;
        g.z = (z1 - z0) * 1024;
        cnt += c.z - a.z;
    } else
        f.z = g.z = 0;

    ftol(f.x * g.z - f.z * g.x, &p.x);
    ftol(g.x, &i.x);
    ftol(f.y * g.z - f.z * g.y, &p.y);
    ftol(g.y, &i.y);
    ftol(f.y * g.x - f.x * g.y, &p.z);
    ftol(g.z, &i.z);

    if (cnt > length)
        cnt = (long) length;
    while (cnt) {
        if (((p.x | p.y) >= 0) && (a.z != c.z)) {
            a.z += d.z;
            p.x -= i.x;
            p.y -= i.y;
        } else if ((p.z >= 0) && (a.x != c.x)) {
            a.x += d.x;
            p.x += i.z;
            p.z -= i.y;
        } else {
            a.y += d.y;
            p.y += i.z;
            p.z += i.x;
        }

        if (isvoxelsolidwrap(server, a.x, a.y, a.z)) {
            *x = a.x;
            *y = a.y;
            *z = a.z;
            return 1;
        }
        cnt--;
    }
    return 0;
}

// original C code

static inline void repositionPlayer(Server* server, uint8 playerID, Vector3f* position)
{
    float f; /* FIXME meaningful name */

    server->player[playerID].movement.eyePos = server->player[playerID].movement.position = *position;
    f = server->player[playerID].lastclimb - ftotclk; /* FIXME meaningful name */
    if (f > -0.25f)
        server->player[playerID].movement.eyePos.z += (f + 0.25f) / 0.25f;
}

static inline void setOrientationVectors(Vector3f* o, Vector3f* s, Vector3f* h)
{
    float f = sqrtf(o->x * o->x + o->y * o->y);
    s->x    = -o->y / f;
    s->y    = o->x / f;
    h->x    = -o->z * s->y;
    h->y    = o->z * s->x;
    h->z    = o->x * s->y - o->y * s->x;
}

void reorientPlayer(Server* server, uint8 playerID, Vector3f* orientation)
{
    server->player[playerID].movement.forwardOrientation = *orientation;
    setOrientationVectors(orientation,
                            &server->player[playerID].movement.strafeOrientation,
                            &server->player[playerID].movement.heightOrientation);
}

int tryUncrouch(Server* server, uint8 playerID)
{
    float x1 = server->player[playerID].movement.position.x + 0.45f;
    float x2 = server->player[playerID].movement.position.x - 0.45f;
    float y1 = server->player[playerID].movement.position.y + 0.45f;
    float y2 = server->player[playerID].movement.position.y - 0.45f;
    float z1 = server->player[playerID].movement.position.z + 2.25f;
    float z2 = server->player[playerID].movement.position.z - 1.35f;

    // first check if player can lower feet (in midair)
    if (server->player[playerID].airborne && !(clipbox(server, x1, y1, z1) || clipbox(server, x1, y2, z1) ||
                                               clipbox(server, x2, y1, z1) || clipbox(server, x2, y2, z1)))
        return (1);
    // then check if they can raise their head
    else if (!(clipbox(server, x1, y1, z2) || clipbox(server, x1, y2, z2) || clipbox(server, x2, y1, z2) ||
               clipbox(server, x2, y2, z2)))
    {
        server->player[playerID].movement.position.z -= 0.9f;
        server->player[playerID].movement.eyePos.z -= 0.9f;
        return (1);
    }
    return (0);
}

// player movement with autoclimb
void boxclipmove(Server* server, uint8 playerID)
{
    float offset, m, f, nx, ny, nz, z;
    long  climb = 0;

    f  = fsynctics * 32.f;
    nx = f * server->player[playerID].movement.velocity.x + server->player[playerID].movement.position.x;
    ny = f * server->player[playerID].movement.velocity.y + server->player[playerID].movement.position.y;

    if (server->player[playerID].crouching) {
        offset = 0.45f;
        m      = 0.9f;
    } else {
        offset = 0.9f;
        m      = 1.35f;
    }

    nz = server->player[playerID].movement.position.z + offset;

    if (server->player[playerID].movement.velocity.x < 0)
        f = -0.45f;
    else
        f = 0.45f;
    z = m;
    while (z >= -1.36f && !clipbox(server, nx + f, server->player[playerID].movement.position.y - 0.45f, nz + z) &&
           !clipbox(server, nx + f, server->player[playerID].movement.position.y + 0.45f, nz + z))
        z -= 0.9f;
    if (z < -1.36f)
        server->player[playerID].movement.position.x = nx;
    else if (!server->player[playerID].crouching && server->player[playerID].movement.forwardOrientation.z < 0.5f &&
             !server->player[playerID].sprinting)
    {
        z = 0.35f;
        while (z >= -2.36f && !clipbox(server, nx + f, server->player[playerID].movement.position.y - 0.45f, nz + z) &&
               !clipbox(server, nx + f, server->player[playerID].movement.position.y + 0.45f, nz + z))
            z -= 0.9f;
        if (z < -2.36f) {
            server->player[playerID].movement.position.x = nx;
            climb                                        = 1;
        } else
            server->player[playerID].movement.velocity.x = 0;
    } else
        server->player[playerID].movement.velocity.x = 0;

    if (server->player[playerID].movement.velocity.y < 0)
        f = -0.45f;
    else
        f = 0.45f;
    z = m;
    while (z >= -1.36f && !clipbox(server, server->player[playerID].movement.position.x - 0.45f, ny + f, nz + z) &&
           !clipbox(server, server->player[playerID].movement.position.x + 0.45f, ny + f, nz + z))
        z -= 0.9f;
    if (z < -1.36f)
        server->player[playerID].movement.position.y = ny;
    else if (!server->player[playerID].crouching && server->player[playerID].movement.forwardOrientation.z < 0.5f &&
             !server->player[playerID].sprinting && !climb)
    {
        z = 0.35f;
        while (z >= -2.36f && !clipbox(server, server->player[playerID].movement.position.x - 0.45f, ny + f, nz + z) &&
               !clipbox(server, server->player[playerID].movement.position.x + 0.45f, ny + f, nz + z))
            z -= 0.9f;
        if (z < -2.36f) {
            server->player[playerID].movement.position.y = ny;
            climb                                        = 1;
        } else
            server->player[playerID].movement.velocity.y = 0;
    } else if (!climb)
        server->player[playerID].movement.velocity.y = 0;

    if (climb) {
        server->player[playerID].movement.velocity.x *= 0.5f;
        server->player[playerID].movement.velocity.y *= 0.5f;
        server->player[playerID].lastclimb = ftotclk;
        nz--;
        m = -1.35f;
    } else {
        if (server->player[playerID].movement.velocity.z < 0)
            m = -m;
        nz += server->player[playerID].movement.velocity.z * fsynctics * 32.f;
    }

    server->player[playerID].airborne = 1;

    if (clipbox(server,
                server->player[playerID].movement.position.x - 0.45f,
                server->player[playerID].movement.position.y - 0.45f,
                nz + m) ||
        clipbox(server,
                server->player[playerID].movement.position.x - 0.45f,
                server->player[playerID].movement.position.y + 0.45f,
                nz + m) ||
        clipbox(server,
                server->player[playerID].movement.position.x + 0.45f,
                server->player[playerID].movement.position.y - 0.45f,
                nz + m) ||
        clipbox(server,
                server->player[playerID].movement.position.x + 0.45f,
                server->player[playerID].movement.position.y + 0.45f,
                nz + m))
    {
        if (server->player[playerID].movement.velocity.z >= 0) {
            server->player[playerID].wade     = server->player[playerID].movement.position.z > 61;
            server->player[playerID].airborne = 0;
        }
        server->player[playerID].movement.velocity.z = 0;
    } else
        server->player[playerID].movement.position.z = nz - offset;

    repositionPlayer(server, playerID, &server->player[playerID].movement.position);
}

long movePlayer(Server* server, uint8 playerID)
{
    float f, f2;

    // move player and perform simple physics (gravity, momentum, friction)
    if (server->player[playerID].jumping) {
        server->player[playerID].jumping             = 0;
        server->player[playerID].movement.velocity.z = -0.36f;
    }

    f = fsynctics; // player acceleration scalar
    if (server->player[playerID].airborne)
        f *= 0.1f;
    else if (server->player[playerID].crouching)
        f *= 0.3f;
    else if ((server->player[playerID].secondary_fire && server->player[playerID].item == 2) ||
             server->player[playerID].sneaking) // Replace me later with ITEM_GUN
        f *= 0.5f;
    else if (server->player[playerID].sprinting)
        f *= 1.3f;

    if ((server->player[playerID].movForward || server->player[playerID].movBackwards) &&
        (server->player[playerID].movLeft || server->player[playerID].movRight))
        f *= SQRT; // if strafe + forward/backwards then limit diagonal velocity

    if (server->player[playerID].movForward) {
        server->player[playerID].movement.velocity.x += server->player[playerID].movement.forwardOrientation.x * f;
        server->player[playerID].movement.velocity.y += server->player[playerID].movement.forwardOrientation.y * f;
    } else if (server->player[playerID].movBackwards) {
        server->player[playerID].movement.velocity.x -= server->player[playerID].movement.forwardOrientation.x * f;
        server->player[playerID].movement.velocity.y -= server->player[playerID].movement.forwardOrientation.y * f;
    }
    if (server->player[playerID].movLeft) {
        server->player[playerID].movement.velocity.x -= server->player[playerID].movement.strafeOrientation.x * f;
        server->player[playerID].movement.velocity.y -= server->player[playerID].movement.strafeOrientation.y * f;
    } else if (server->player[playerID].movRight) {
        server->player[playerID].movement.velocity.x += server->player[playerID].movement.strafeOrientation.x * f;
        server->player[playerID].movement.velocity.y += server->player[playerID].movement.strafeOrientation.y * f;
    }

    f = fsynctics + 1;
    server->player[playerID].movement.velocity.z += fsynctics;
    server->player[playerID].movement.velocity.z /= f; // air friction
    if (server->player[playerID].wade)
        f = fsynctics * 6.f + 1; // water friction
    else if (!server->player[playerID].airborne)
        f = fsynctics * 4.f + 1; // ground friction
    server->player[playerID].movement.velocity.x /= f;
    server->player[playerID].movement.velocity.y /= f;
    f2 = server->player[playerID].movement.velocity.z;
    boxclipmove(server, playerID);
    // hit ground... check if hurt
    if (!server->player[playerID].movement.velocity.z && (f2 > FALL_SLOW_DOWN)) {
        // slow down on landing
        server->player[playerID].movement.velocity.x *= 0.5f;
        server->player[playerID].movement.velocity.y *= 0.5f;

        // return fall damage
        if (f2 > FALL_DAMAGE_VELOCITY) {
            f2 -= FALL_DAMAGE_VELOCITY;
            return ((long) (f2 * f2 * FALL_DAMAGE_SCALAR));
        }

        return (-1); // no fall damage but play fall sound
    }

    return (0); // no fall damage
}

int moveGrenade(Server* server, Grenade* grenade)
{
    Vector3f fpos = grenade->position; // old position
    // do velocity & gravity (friction is negligible)
    float f = fsynctics * 32;
    grenade->velocity.z += fsynctics;
    grenade->position.x += grenade->velocity.x * f;
    grenade->position.y += grenade->velocity.y * f;
    grenade->position.z += grenade->velocity.z * f;
    // do rotation
    // FIX ME: Loses orientation after 45 degree bounce off wall
    //  if(g->v.x > 0.1f || g->v.x < -0.1f || g->v.y > 0.1f || g->v.y < -0.1f)
    //  {
    //  f *= -0.5;
    //  }
    // make it bounce (accurate)
    Vector3l lp;
    lp.x = (long) floor(grenade->position.x);
    lp.y = (long) floor(grenade->position.y);
    lp.z = (long) floor(grenade->position.z);

    if (!clipworld(server, lp.x, lp.y, lp.z)) {
        return 0; // we didn't hit anything, no collision
    } else {      // hit a wall
        static const float BOUNCE_SOUND_THRESHOLD = 1.1f;

        int ret = 1;
        if (fabsf(grenade->velocity.x) > BOUNCE_SOUND_THRESHOLD || fabsf(grenade->velocity.y) > BOUNCE_SOUND_THRESHOLD ||
            fabsf(grenade->velocity.z) > BOUNCE_SOUND_THRESHOLD)
            ret = 2; // play sound

        Vector3l lp2;
        lp2.x = (long) floor(fpos.x);
        lp2.y = (long) floor(fpos.y);
        lp2.z = (long) floor(fpos.z);
        if (lp.z != lp2.z && ((lp.x == lp2.x && lp.y == lp2.y) || !clipworld(server, lp.x, lp.y, lp2.z)))
            grenade->velocity.z = -grenade->velocity.z;
        else if (lp.x != lp2.x && ((lp.y == lp2.y && lp.z == lp2.z) || !clipworld(server, lp2.x, lp.y, lp.z)))
            grenade->velocity.x = -grenade->velocity.x;
        else if (lp.y != lp2.y && ((lp.x == lp2.x && lp.z == lp2.z) || !clipworld(server, lp.x, lp2.y, lp.z)))
            grenade->velocity.y = -grenade->velocity.y;
        grenade->position = fpos; // set back to old position
        grenade->velocity.x *= 0.36f;
        grenade->velocity.y *= 0.36f;
        grenade->velocity.z *= 0.36f;
        return ret;
    }
}
// C interface

void setPhysicsGlobals(float time, float dt)
{
    ftotclk   = time;
    fsynctics = dt;
}
