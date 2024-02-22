#ifndef DOS2ANSI_BUFFEREDWRITER_H
#define DOS2ANSI_BUFFEREDWRITER_H

#include "decl.h"

#include <stddef.h>

C_CLASS_DECL(Stream);

Stream *BufferedWriter_create(Stream *out, size_t bufsize)
    ATTR_NONNULL((1)) ATTR_RETNONNULL;

void BufferedWriter_discard(Stream *stream);

#endif
