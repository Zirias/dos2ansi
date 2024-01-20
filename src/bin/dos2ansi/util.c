#include "util.h"

#include <stdlib.h>

void *xmalloc(size_t size)
{
    void *m = malloc(size);
    if (!m) abort();
    return m;
}

void *xrealloc(void *ptr, size_t size)
{
    void *m = realloc(ptr, size);
    if (!m) abort();
    return m;
}

