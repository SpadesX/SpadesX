#ifndef TOMLHELPERS_H
#define TOMLHELPERS_H

#include <Util/Log.h>
#include <tomlc99/toml.h>
#include <Server/Structs/MapStruct.h>

#define TOMLH_READ_FROM_FILE(toml, path)                                       \
    {                                                                          \
        char  toml##_error[200];                                               \
        FILE* toml##_fp = fopen(path, "r");                                    \
        if (!toml##_fp) {                                                      \
            LOG_ERROR("Cannot find TOML file %s", path);                       \
            exit(EXIT_FAILURE);                                                \
        }                                                                      \
        toml = toml_parse_file(toml##_fp, toml##_error, sizeof(toml##_error)); \
        fclose(toml##_fp);                                                     \
        if (!toml) {                                                           \
            LOG_ERROR("Failed to read %s.", path);                             \
            LOG_ERROR("%s", toml##_error);                                     \
            exit(EXIT_FAILURE);                                                \
        }                                                                      \
    }

#define TOMLH_GET_TABLE(toml, table, table_name)             \
    {                                                        \
        table = toml_table_in(toml, table_name);             \
        if (!table) {                                        \
            LOG_ERROR("Cannot find table [%s]", table_name); \
            exit(EXIT_FAILURE);                              \
        }                                                    \
    }

#define TOMLH_GET_VAR(type, table, out, name, cast, fallback, optional)  \
    {                                                                    \
        toml_datum_t out##_datum = toml_##type##_in(table, name);        \
        if (!out##_datum.ok) {                                           \
            if (!optional) {                                             \
                LOG_WARNING("Cannot find [" #type "]" name " in config." \
                            " Falling back to " #fallback);              \
            }                                                            \
            out = fallback;                                              \
        } else {                                                         \
            out = cast;                                                  \
        }                                                                \
    }

#define TOMLH_GET_INT(table, out, name, fallback, optional) \
    TOMLH_GET_VAR(int, table, out, name, (int) out##_datum.u.i, fallback, optional);

#define TOMLH_GET_INT_ARRAY(table, out, name, size, fallback, optional)                                      \
    {                                                                                                        \
        toml_array_t* out##_array      = toml_array_in(table, name);                                         \
        size_t        out##_array_size = 0;                                                                  \
        if (out##_array) {                                                                                   \
            out##_array_size = toml_array_nelem(out##_array);                                                \
        }                                                                                                    \
        for (size_t i = 0; i < size; i++) {                                                                  \
            if (i < out##_array_size) {                                                                      \
                toml_datum_t out##_array_elm = toml_int_at(out##_array, i);                                  \
                if (!out##_array_elm.ok) {                                                                   \
                    LOG_ERROR("Failed to read " #name "[%zu] from TOML", i);                                 \
                    exit(EXIT_FAILURE);                                                                      \
                }                                                                                            \
                out[i] = (int) out##_array_elm.u.i;                                                          \
            } else {                                                                                         \
                if (!optional) {                                                                             \
                    LOG_WARNING("Config option " name "[%i] not found, falling back to %i", i, fallback[i]); \
                }                                                                                            \
                out[i] = fallback[i];                                                                        \
            }                                                                                                \
        }                                                                                                    \
    }

#define TOMLH_GET_STRING_ARRAY_AS_DL(table, out, size_out, name, optional)       \
    {                                                                            \
        toml_array_t* out##_array = toml_array_in(table, name);                  \
        if (!out##_array) {                                                      \
            if (optional) {                                                      \
                size_out = 0;                                                    \
                goto out##_done_reading;                                         \
            } else {                                                             \
                LOG_ERROR("Failed to read string array '%s' from TOML", name);   \
                exit(EXIT_FAILURE);                                              \
            }                                                                    \
        }                                                                        \
        size_out = toml_array_nelem(out##_array);                                \
        for (size_t i = 0; i < size_out; i++) {                                  \
            toml_datum_t out##_array_elm = toml_string_at(out##_array, i);       \
            if (!out##_array_elm.ok) {                                           \
                LOG_ERROR("Failed to read " #name "[%zu] from TOML", i);         \
                exit(EXIT_FAILURE);                                              \
            }                                                                    \
            string_node_t* out##_string = spadesx_malloc(sizeof(string_node_t)); \
            out##_string->string        = (char*) out##_array_elm.u.s;           \
            DL_APPEND(out, out##_string);                                        \
        }                                                                        \
    }                                                                            \
    out##_done_reading:

#define TOMLH_GET_STRING(table, out, name, fallback, optional) \
    TOMLH_GET_VAR(string, table, out, name, out##_datum.u.s, fallback, optional);

#define TOMLH_GET_BOOL(table, out, name, fallback, optional) \
    TOMLH_GET_VAR(bool, table, out, name, (int) out##_datum.u.b, fallback, optional);

#define TOMLH_GET_RGB_COLOR(table, out, name, fallback, optional)                 \
    {                                                                             \
        uint8_t out##_array_tmp[3];                                               \
        TOMLH_GET_INT_ARRAY(table, out##_array_tmp, name, 3, fallback, optional); \
        out.r = out##_array_tmp[0];                                               \
        out.g = out##_array_tmp[1];                                               \
        out.b = out##_array_tmp[2];                                               \
    }

#endif
