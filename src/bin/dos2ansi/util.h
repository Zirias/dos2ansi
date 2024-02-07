#ifndef DOS2ANSI_UTIL_H
#define DOS2ANSI_UTIL_H

#include "decl.h"

#include <stddef.h>

void *xmalloc(size_t size) ATTR_MALLOC ATTR_ALLOCSZ((1)) ATTR_RETNONNULL;
void *xrealloc(void *ptr, size_t size) ATTR_ALLOCSZ((2)) ATTR_RETNONNULL;

#endif
