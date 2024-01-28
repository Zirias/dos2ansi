#ifndef DOS2ANSI_STREAM_H
#define DOS2ANSI_STREAM_H

#include "decl.h"

#include <stdio.h>

C_CLASS_DECL(Stream);
C_CLASS_DECL(StreamWriter);

#define SS_OK 0
#define SS_EOF 1
#define SS_ERROR 2

struct StreamWriter
{
    size_t  (*write)(StreamWriter *self, const void *ptr, size_t size);
    int	    (*flush)(StreamWriter *self);
    int	    (*status)(const StreamWriter *self);
    void    (*destroy)(StreamWriter *self);
    Stream  *stream;
};

Stream *Stream_createMemory(void);
Stream *Stream_createFile(FILE *file);
Stream *Stream_createWriter(StreamWriter *writer);

size_t Stream_write(Stream *self, const void *ptr, size_t sz);
size_t Stream_read(Stream *self, void *ptr, size_t sz);
int Stream_flush(Stream *self);
int Stream_status(const Stream *self);

void Stream_destroy(Stream *self);

#endif
