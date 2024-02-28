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

typedef enum StreamSeekStart
{
    SSS_START	= 0,	    /* Seek from the beginning of the stream */
    SSS_POS	= 1,	    /* Seek from current reading position */
    SSS_END	= 2	    /* Seek from the end of the stream */
} StreamSeekStart;

#if defined(USE_WIN32)
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
    size_t  (*read)(StreamReader *self, void *ptr, size_t size)
	    CMETHOD ATTR_NONNULL((2));
    int	    (*status)(const StreamReader *self) CMETHOD;
    void    (*destroy)(StreamReader *self);
    Stream  *stream;
};

struct StreamWriter
{
    size_t  (*write)(StreamWriter *self, const void *ptr, size_t size)
	    CMETHOD ATTR_NONNULL((2));
    int	    (*flush)(StreamWriter *self, int sys) CMETHOD;
    int	    (*status)(const StreamWriter *self) CMETHOD;
    void    (*destroy)(StreamWriter *self);
    Stream  *stream;
};

Stream *Stream_createMemory(size_t maxsize) ATTR_RETNONNULL;
Stream *Stream_fromStream(Stream *in, size_t maxsize) ATTR_NONNULL((1));
Stream *Stream_fromFile(FILEHANDLE file, FileOpenFlags flags) ATTR_RETNONNULL;
Stream *Stream_createStandard(StandardStreamType type) ATTR_RETNONNULL;
Stream *Stream_openFile(const char *filename, FileOpenFlags flags);
Stream *Stream_createReader(StreamReader *reader, const void *magic)
    ATTR_NONNULL((1)) ATTR_NONNULL((2)) ATTR_RETNONNULL;
Stream *Stream_createWriter(StreamWriter *writer, const void *magic)
    ATTR_NONNULL((1)) ATTR_NONNULL((2)) ATTR_RETNONNULL;

FILEHANDLE Stream_file(Stream *self) CMETHOD ATTR_PURE;
StreamReader *Stream_reader(Stream *self, const void *magic)
    CMETHOD ATTR_NONNULL((2)) ATTR_PURE;
StreamWriter *Stream_writer(Stream *self, const void *magic)
    CMETHOD ATTR_NONNULL((2)) ATTR_PURE;

size_t Stream_write(Stream *self, const void *ptr, size_t sz)
    CMETHOD ATTR_NONNULL((2));
int Stream_putc(Stream *self, int c) CMETHOD;
size_t Stream_puts(Stream *self, const char *str)
    CMETHOD ATTR_NONNULL((2));
#ifdef va_start
int Stream_vprintf(Stream *self, const char *format, va_list ap)
    CMETHOD ATTR_NONNULL((2)) ATTR_NONNULL((3));
#endif
int Stream_printf(Stream *self, const char *format, ...)
    CMETHOD ATTR_NONNULL((2)) ATTR_FORMAT((printf, 2, 3));
size_t Stream_read(Stream *self, void *ptr, size_t sz)
    CMETHOD ATTR_NONNULL((2));
int Stream_getc(Stream *self) CMETHOD;
int Stream_flush(Stream *self, int sys) CMETHOD;
long Stream_size(Stream *self) CMETHOD;
long Stream_seek(Stream *self, StreamSeekStart start, long offset) CMETHOD;
StreamStatus Stream_status(const Stream *self) CMETHOD;

void Stream_destroy(Stream *self);

#endif
