#ifndef DOS2ANSI_STREAM_H
#define DOS2ANSI_STREAM_H

#include "decl.h"

#include <stdio.h>

C_CLASS_DECL(Stream);
C_CLASS_DECL(StreamReader);
C_CLASS_DECL(StreamWriter);

typedef enum FileOpenFlags
{
    FOF_NONE	= 0,
    FOF_READ	= 1 << 0,
    FOF_WRITE	= 1 << 1
} FileOpenFlags;

typedef enum StandardStreamType
{
    SST_STDIN,
    SST_STDOUT,
    SST_STDERR
} StandardStreamType;

#define SS_OK 0
#define SS_EOF 1
#define SS_ERROR 2

#ifdef USE_POSIX
#define FILETYPE int
#define NOTAFILE (-1)
#else
#define FILETYPE FILE *
#define NOTAFILE ((void *)0)
#endif

struct StreamReader
{
    size_t  (*read)(StreamReader *self, void *ptr, size_t size);
    int	    (*status)(const StreamReader *self);
    void    (*destroy)(StreamReader *self);
    Stream  *stream;
};

struct StreamWriter
{
    size_t  (*write)(StreamWriter *self, const void *ptr, size_t size);
    int	    (*flush)(StreamWriter *self);
    int	    (*status)(const StreamWriter *self);
    void    (*destroy)(StreamWriter *self);
    Stream  *stream;
};

Stream *Stream_createMemory(void);
Stream *Stream_openFile(const char *filename, FileOpenFlags flags);
Stream *Stream_createStandard(StandardStreamType type);
Stream *Stream_createReader(StreamReader *reader, const void *magic);
Stream *Stream_createWriter(StreamWriter *writer, const void *magic);

FILETYPE Stream_file(Stream *self);
StreamReader *Stream_reader(Stream *self, const void *magic);
StreamWriter *Stream_writer(Stream *self, const void *magic);

size_t Stream_write(Stream *self, const void *ptr, size_t sz);
size_t Stream_read(Stream *self, void *ptr, size_t sz);
int Stream_flush(Stream *self);
int Stream_status(const Stream *self);

void Stream_destroy(Stream *self);

#endif
