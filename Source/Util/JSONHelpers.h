#ifndef JSONHELPERS_H
#define JSONHELPERS_H

#define READ_VAR_FROM_JSON(type, fmt, pj, var, jsonvar, description, fallback) {\
    struct json_object* _ ## var; \
    if (!json_object_object_get_ex(pj, #jsonvar, &_ ## var)){ \
    LOG_WARNING("Failed to find " description " in config, falling back to " fmt, fallback); \
    var = fallback; \
    } else var = json_object_get_ ## type (_ ## var);}

#define READ_INT_FROM_JSON(pj, var, jsonvar, description, fallback) READ_VAR_FROM_JSON(int, "%i", pj, var, jsonvar, description, fallback)
#define READ_STR_FROM_JSON(pj, var, jsonvar, description, fallback) READ_VAR_FROM_JSON(string, "\"%s\"", pj, var, jsonvar, description, fallback)

#define READ_INT_ARR_FROM_JSON(pj, var, jsonvar, description, fallback, fbsize) {\
    struct json_object* _ ## var; \
    if (!json_object_object_get_ex(pj, #jsonvar, &_ ## var)){ \
    LOG_WARNING("Failed to find " description " in config, falling back to " #fallback); \
    for (int i = 0; i < fbsize; ++i) var[i] = fallback[i]; \
    } else for (int i = 0; i < fbsize; ++i) var[i] = json_object_get_int(json_object_array_get_idx(_ ## var, i));}

#endif
