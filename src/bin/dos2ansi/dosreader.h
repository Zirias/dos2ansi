#ifndef DOS2ANSI_DOSREADER_H
#define DOS2ANSI_DOSREADER_H

#include "decl.h"

#include <stddef.h>

#define SS_DOSEOF -1

C_CLASS_DECL(Stream);

Stream *DosReader_create(Stream *in, size_t bufsize, int ignoreeof);
int DosReader_seekAfterEof(Stream *stream);

#endif
