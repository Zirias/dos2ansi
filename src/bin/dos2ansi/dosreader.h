#ifndef DOS2ANSI_DOSREADER_H
#define DOS2ANSI_DOSREADER_H

#include "decl.h"

#include <stddef.h>

C_CLASS_DECL(Stream);

Stream *DosReader_create(Stream *in, size_t bufsize, long insz, int ignoreeof)
    ATTR_NONNULL((1)) ATTR_RETNONNULL;

#endif
