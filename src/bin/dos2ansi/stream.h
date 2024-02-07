#ifndef DOS2ANSI_STREAM_H
#define DOS2ANSI_STREAM_H

#include "decl.h"

#include <stddef.h>

C_CLASS_DECL(Stream);
C_CLASS_DECL(StreamReader);
C_CLASS_DECL(StreamWriter);

typedef enum FileOpenFlags
{
    FOF_NONE	= 0,	    /* Invalid */
    FOF_READ	= 1 << 0,   /* Request read access */
    FOF_WRITE	= 1 << 1,   /* Request write access, truncate if exists */
    FOF_NOCLOSE	= 1 << 15   /* Don't close file on stream destruction */
} FileOpenFlags;

typedef enum StandardStreamType
{
    SST_STDIN	= 0,	    /* FileStream for standard input */
    SST_STDOUT	= 1,	    /* FileStream for standard output */
    SST_STDERR	= 2	    /* FileStream for standard error */
} StandardStreamType;

typedef enum StreamStatus
{
    SS_RESERVED	= -1,	    /* Negative values for reader/writer extensions */
    SS_OK	= 0,	    /* Stream is usable */
    SS_EOF	= 1,	    /* Stream reached the end */
    SS_ERROR	= 2	    /* An error occured */
} StreamStatus;

#if defined(_WIN32)
typedef void *FILEHANDLE;	    /* Use a HANDLE in client code */
#  define NOTAFILE ((FILEHANDLE)-1)
#elif defined(USE_POSIX)
typedef int FILEHANDLE;
#  define NOTAFILE ((FILEHANDLE)-1)
#else
typedef void *FILEHANDLE;	    /* Use a FILE * in client code */
#  define NOTAFILE ((FILEHANDLE)0)
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
Stream *Stream_fromFile(FILEHANDLE file, FileOpenFlags flags);
Stream *Stream_createStandard(StandardStreamType type);
Stream *Stream_openFile(const char *filename, FileOpenFlags flags);
Stream *Stream_createReader(StreamReader *reader, const void *magic);
Stream *Stream_createWriter(StreamWriter *writer, const void *magic);

FILEHANDLE Stream_file(Stream *self);
StreamReader *Stream_reader(Stream *self, const void *magic);
StreamWriter *Stream_writer(Stream *self, const void *magic);

size_t Stream_write(Stream *self, const void *ptr, size_t sz);
size_t Stream_read(Stream *self, void *ptr, size_t sz);
int Stream_flush(Stream *self);
StreamStatus Stream_status(const Stream *self);

void Stream_destroy(Stream *self);

#endif
