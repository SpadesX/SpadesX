#ifndef JSONHELPERS_H
#define JSONHELPERS_H

#include <Util/Log.h>

#define READ_VAR_FROM_JSON(type, fmt, pj, var, jsonvar, description, fallback, optional)                 \
    {                                                                                                    \
        struct json_object* _##var;                                                                      \
        if (!json_object_object_get_ex(pj, #jsonvar, &_##var)) {                                         \
            if (optional == 0) {                                                                         \
                LOG_WARNING("Failed to find " description " in config, falling back to " fmt, fallback); \
            }                                                                                            \
            var = fallback;                                                                              \
        } else                                                                                           \
            var = json_object_get_##type(_##var);                                                        \
    }

#define READ_INT_FROM_JSON(pj, var, jsonvar, description, fallback, optional) \
    READ_VAR_FROM_JSON(int, "%i", pj, var, jsonvar, description, fallback, optional)
#define READ_DOUBLE_FROM_JSON(pj, var, jsonvar, description, fallback, optional) \
    READ_VAR_FROM_JSON(int, "%lf", pj, var, jsonvar, description, fallback, optional)
#define READ_STR_FROM_JSON(pj, var, jsonvar, description, fallback, optional) \
    READ_VAR_FROM_JSON(string, "\"%s\"", pj, var, jsonvar, description, fallback, optional)

#define READ_INT_ARR_FROM_JSON(pj, var, jsonvar, description, fallback, fbsize, optional)            \
    {                                                                                                \
        struct json_object* _##var;                                                                  \
        if (!json_object_object_get_ex(pj, #jsonvar, &_##var)) {                                     \
            if (optional == 0) {                                                                     \
                LOG_WARNING("Failed to find " description " in config, falling back to " #fallback); \
            }                                                                                        \
            for (int i = 0; i < fbsize; ++i)                                                         \
                var[i] = fallback[i];                                                                \
        } else                                                                                       \
            for (int i = 0; i < fbsize; ++i)                                                         \
                var[i] = json_object_get_int(json_object_array_get_idx(_##var, i));                  \
    }

#endif
