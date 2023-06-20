#include <Util/Alloc.h>
#include <Util/Log.h>
#include <stdlib.h>

inline void* spadesx_malloc(size_t size)
{
    void* ptr = malloc(size);
    if (ptr == NULL) {
        LOG_ERROR("malloc() failed to allocate %zu bytes of memory", size);
        abort();
    }

    return ptr;
}

inline void* spadesx_calloc(size_t num, size_t size)
{
    void* ptr = calloc(num, size);
    if (ptr == NULL) {
        LOG_ERROR("calloc() failed to allocate %zu bytes of memory", num * size);
        abort();
    }

    return ptr;
}