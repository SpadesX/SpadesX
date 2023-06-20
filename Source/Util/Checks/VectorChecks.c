#include <Util/Checks/VectorChecks.h>
#include <Util/Types.h>
#include <math.h>

inline uint8_t valid_vec3f(vector3f_t vec)
{
    return !(isnan(vec.x) || isnan(vec.y) || isnan(vec.z) ||
             isinf(vec.x) || isinf(vec.y) || isinf(vec.z));
}
