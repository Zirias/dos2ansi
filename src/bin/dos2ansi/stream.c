#include "stream.h"

#include "util.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef USE_POSIX
#  include <fcntl.h>
#  include <unistd.h>
#endif

#ifdef _WIN32
#  include <fcntl.h>
#  include <io.h>
#  define BINMODE(f) _setmode(_fileno(f), _O_BINARY)
#else
#  define BINMODE(f) (void)(f)
#endif

#define MS_CHUNKSZ 1024

struct Stream
{
    size_t size;
};

typedef struct MemoryStream
{
    struct Stream base;
    size_t readpos;
    size_t writepos;
    unsigned char *mem;
} MemoryStream;

#define T_FILESTREAM 1
typedef struct FileStream
{
    struct Stream base;
#ifdef USE_POSIX
    int fd;
    int status;
#else
    FILE *file;
#endif
    FileOpenFlags flags;
} FileStream;

#define T_READERSTREAM 2
typedef struct ReaderStream
{
    struct Stream base;
    const void *magic;
    StreamReader *reader;
} ReaderStream;

#define T_WRITERSTREAM 3
typedef struct WriterStream
{
    struct Stream base;
    const void *magic;
    StreamWriter *writer;
} WriterStream;

Stream *Stream_createMemory(void)
{
    MemoryStream *self = xmalloc(sizeof *self);
    self->base.size = MS_CHUNKSZ;
    self->readpos = 0;
    self->writepos = 0;
    self->mem = xmalloc(self->base.size * sizeof *self->mem);
    return (Stream *)self;
}

Stream *Stream_createStandard(StandardStreamType type)
{
    FileStream *self = xmalloc(sizeof *self);
    self->base.size = T_FILESTREAM;
#ifdef USE_POSIX
    switch (type)
    {
	case SST_STDIN:
	    self->fd = STDIN_FILENO;
	    self->flags = FOF_READ;
	    break;
	case SST_STDOUT:
	    self->fd = STDOUT_FILENO;
	    self->flags = FOF_WRITE;
	    break;
	case SST_STDERR:
	    self->fd = STDERR_FILENO;
	    self->flags = FOF_WRITE;
	    break;
    }
    self->status = SS_OK;
#else
    FILE *f;
    switch (type)
    {
	case SST_STDIN:
	   f = stdin;
	   break;
	case SST_STDOUT:
	   f = stdout;
	   break;
	case SST_STDERR:
	   f = stderr;
	   break;
    }
    setvbuf(f, 0, _IONBF, 0);
    BINMODE(f);
    self->file = f;
#endif
    return (Stream *)self;
}

Stream *Stream_openFile(const char *filename, FileOpenFlags flags)
{
    if (!(flags & (FOF_READ|FOF_WRITE))) return 0;
#ifdef USE_POSIX
    int openflags = 0;
    if (flags & FOF_WRITE)
    {
	if (flags & FOF_READ) openflags |= O_RDWR;
	else openflags |= O_WRONLY;
	openflags |= O_CREAT|O_TRUNC;
    }
    else if (flags & FOF_READ) openflags |= O_RDONLY;
    int fd = open(filename, openflags);
    if (fd < 0) return 0;
    FileStream *self = xmalloc(sizeof *self);
    self->fd = fd;
    self->status = SS_OK;
#else
    char mode[4] = "";
    char *mptr = mode;
    if (flags & FOF_READ) *mptr++ = 'r';
    if (flags & FOF_WRITE) *mptr++ = 'w';
    *mptr = 'b';
    FILE *file = fopen(filename, mode);
    if (!file) return 0;
    setvbuf(file, 0, _IONBF, 0);
    FileStream *self = xmalloc(sizeof *self);
    self->file = file;
#endif
    self->base.size = T_FILESTREAM;
    self->flags = flags;
    return (Stream *)self;
}

Stream *Stream_createReader(StreamReader *reader, const void *magic)
{
    if (!magic || !reader->read ||
	    (!reader->status && !reader->stream)) return 0;
    ReaderStream *self = xmalloc(sizeof *self);
    self->base.size = T_READERSTREAM;
    self->magic = magic;
    self->reader = reader;
    return (Stream *)self;
}

Stream *Stream_createWriter(StreamWriter *writer, const void *magic)
{
    if (!magic || !writer->write ||
	    (!writer->status && !writer->stream)) return 0;
    WriterStream *self = xmalloc(sizeof *self);
    self->base.size = T_WRITERSTREAM;
    self->magic = magic;
    self->writer = writer;
    return (Stream *)self;
}

StreamReader *Stream_reader(Stream *self, const void *magic)
{
    if (!magic || self->size != T_READERSTREAM) return 0;
    ReaderStream *rs = (ReaderStream *)self;
    if (rs->magic == magic) return rs->reader;
    if (!rs->reader->stream) return 0;
    return Stream_reader(rs->reader->stream, magic);
}

StreamWriter *Stream_writer(Stream *self, const void *magic)
{
    if (!magic || self->size != T_WRITERSTREAM) return 0;
    WriterStream *ws = (WriterStream *)self;
    if (ws->magic == magic) return ws->writer;
    if (!ws->writer->stream) return 0;
    return Stream_writer(ws->writer->stream, magic);
}

static size_t MemoryStream_write(MemoryStream *self,
	const void *ptr, size_t sz)
{
    size_t newpos = self->writepos + sz;
    if (newpos > self->base.size)
    {
	size_t newsz = newpos;
	size_t fragsz = newsz % MS_CHUNKSZ;
	if (fragsz) newsz += (MS_CHUNKSZ - fragsz);
	self->mem = xrealloc(self->mem, newsz * sizeof *self->mem);
	self->base.size = newsz;
    }
    memcpy(self->mem + self->writepos, ptr, sz);
    self->writepos = newpos;
    return sz;
}

static size_t FileStream_write(FileStream *self, const void *ptr, size_t sz)
{
    if (!(self->flags & FOF_WRITE)) return 0;
#ifdef USE_POSIX
    if (self->status != SS_OK) return 0;
    ssize_t rc = write(self->fd, ptr, sz);
    if (rc < 0)
    {
	self->status = SS_ERROR;
	return 0;
    }
    if (rc == 0) self->status = SS_EOF;
    return rc;
#else
    return fwrite(ptr, 1, sz, self->file);
#endif
}

static size_t WriterStream_write(WriterStream *self,
	const void *ptr, size_t sz)
{
    return self->writer->write(self->writer, ptr, sz);
}

size_t Stream_write(Stream *self, const void *ptr, size_t sz)
{
    switch (self->size)
    {
	case T_FILESTREAM:
	    return FileStream_write((FileStream *)self, ptr, sz);
	case T_READERSTREAM:
	    return 0;
	case T_WRITERSTREAM:
	    return WriterStream_write((WriterStream *)self, ptr, sz);
	default:
	    return MemoryStream_write((MemoryStream *)self, ptr, sz);
    }
}

static size_t MemoryStream_read(MemoryStream *self, void *ptr, size_t sz)
{
    size_t avail = self->writepos - self->readpos;
    if (sz > avail) sz = avail;
    memcpy(ptr, self->mem + self->readpos, sz);
    self->readpos += sz;
    return sz;
}

static size_t FileStream_read(FileStream *self, void *ptr, size_t sz)
{
    if (!(self->flags & FOF_READ)) return 0;
#ifdef USE_POSIX
    if (self->status != SS_OK) return 0;
    ssize_t rc = read(self->fd, ptr, sz);
    if (rc < 0)
    {
	self->status = SS_ERROR;
	return 0;
    }
    if (rc == 0) self->status = SS_EOF;
    return rc;
#else
    return fread(ptr, 1, sz, self->file);
#endif
}

static size_t ReaderStream_read(ReaderStream *self, void *ptr, size_t sz)
{
    return self->reader->read(self->reader, ptr, sz);
}

size_t Stream_read(Stream *self, void *ptr, size_t sz)
{
    switch (self->size)
    {
	case T_FILESTREAM:
	    return FileStream_read((FileStream *)self, ptr, sz);
	case T_READERSTREAM:
	    return ReaderStream_read((ReaderStream *)self, ptr, sz);
	case T_WRITERSTREAM:
	    return 0;
	default:
	    return MemoryStream_read((MemoryStream *)self, ptr, sz);
    }
}

static int FileStream_flush(FileStream *self)
{
    if (!(self->flags & FOF_WRITE)) return EOF;
#ifdef USE_POSIX
    return 0;
#else
    return fflush(self->file);
#endif
}

static int WriterStream_flush(WriterStream *self)
{
    if (self->writer->flush) return self->writer->flush(self->writer);
    else if (self->writer->stream) return Stream_flush(self->writer->stream);
    else return EOF;
}

int Stream_flush(Stream *self)
{
    switch (self->size)
    {
	case T_FILESTREAM: return FileStream_flush((FileStream *)self);
	case T_WRITERSTREAM: return WriterStream_flush((WriterStream *)self);
	default: return EOF;
    }
}

static int MemoryStream_status(const MemoryStream *self)
{
    return self->readpos == self->writepos ? SS_EOF : SS_OK;
}

static int FileStream_status(const FileStream *self)
{
#ifdef USE_POSIX
    return self->status;
#else
    if (ferror(self->file)) return SS_ERROR;
    else if (feof(self->file)) return SS_EOF;
    else return SS_OK;
#endif
}

static int ReaderStream_status(const ReaderStream *self)
{
    if (self->reader->status) return self->reader->status(self->reader);
    return Stream_status(self->reader->stream);
}

static int WriterStream_status(const WriterStream *self)
{
    if (self->writer->status) return self->writer->status(self->writer);
    return Stream_status(self->writer->stream);
}

int Stream_status(const Stream *self)
{
    switch (self->size)
    {
	case T_FILESTREAM:
	    return FileStream_status((const FileStream *)self);
	case T_READERSTREAM:
	    return ReaderStream_status((const ReaderStream *)self);
	case T_WRITERSTREAM:
	    return WriterStream_status((const WriterStream *)self);
	default:
	    return MemoryStream_status((const MemoryStream *)self);
    }
}

static void MemoryStream_destroy(MemoryStream *self)
{
    free(self->mem);
}

static void FileStream_destroy(FileStream *self)
{
#ifdef USE_POSIX
    if (self->fd != STDIN_FILENO && self->fd != STDOUT_FILENO
	    && self->fd != STDERR_FILENO)
    {
	close(self->fd);
    }
#else
    if (self->file != stdin && self->file != stdout && self->file != stderr)
    {
	fclose(self->file);
    }
#endif
}

static void ReaderStream_destroy(ReaderStream *self)
{
    if (self->reader->destroy) self->reader->destroy(self->reader);
    else
    {
	Stream_destroy(self->reader->stream);
	free(self->reader);
    }
}

static void WriterStream_destroy(WriterStream *self)
{
    if (self->writer->destroy) self->writer->destroy(self->writer);
    else
    {
	WriterStream_flush(self);
	Stream_destroy(self->writer->stream);
	free(self->writer);
    }
}

void Stream_destroy(Stream *self)
{
    if (!self) return;
    switch (self->size)
    {
	case T_FILESTREAM: FileStream_destroy((FileStream *)self); break;
	case T_READERSTREAM: ReaderStream_destroy((ReaderStream *)self); break;
	case T_WRITERSTREAM: WriterStream_destroy((WriterStream *)self); break;
	default: MemoryStream_destroy((MemoryStream *)self);
    }
    free(self);
}

