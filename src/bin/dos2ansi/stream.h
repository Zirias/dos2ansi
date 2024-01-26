#ifndef DOS2ANSI_STREAM_H
#define DOS2ANSI_STREAM_H

#include "decl.h"

#include <stdio.h>

C_CLASS_DECL(Stream);

#define SS_OK 0
#define SS_EOF 1
#define SS_ERROR 2

Stream *Stream_createMemory(void);
Stream *Stream_createFile(FILE *file);

size_t Stream_write(Stream *self, const void *ptr, size_t sz);
size_t Stream_read(Stream *self, void *ptr, size_t sz);
int Stream_status(const Stream *self);

void Stream_destroy(Stream *self);

#endif
