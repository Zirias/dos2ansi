#include "stream.h"

#include "util.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
    FILE *file;
} FileStream;

#define T_WRITERSTREAM 2
typedef struct WriterStream
{
    struct Stream base;
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

Stream *Stream_createFile(FILE *file)
{
    FileStream *self = xmalloc(sizeof *self);
    self->base.size = T_FILESTREAM;
    self->file = file;
    return (Stream *)self;
}

Stream *Stream_createWriter(StreamWriter *writer)
{
    if (!writer->write || (!writer->status && !writer->stream)) return 0;
    WriterStream *self = xmalloc(sizeof *self);
    self->base.size = T_WRITERSTREAM;
    self->writer = writer;
    return (Stream *)self;
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
    return fwrite(ptr, 1, sz, self->file);
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
    return fread(ptr, 1, sz, self->file);
}

size_t Stream_read(Stream *self, void *ptr, size_t sz)
{
    switch (self->size)
    {
	case T_FILESTREAM:
	    return FileStream_read((FileStream *)self, ptr, sz);
	case T_WRITERSTREAM:
	    return 0;
	default:
	    return MemoryStream_read((MemoryStream *)self, ptr, sz);
    }
}

static int FileStream_flush(FileStream *self)
{
    return fflush(self->file);
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
    if (ferror(self->file)) return SS_ERROR;
    else if (feof(self->file)) return SS_EOF;
    else return SS_OK;
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
    if (self->file != stdin && self->file != stdout && self->file != stderr)
    {
	fclose(self->file);
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
	case T_WRITERSTREAM: WriterStream_destroy((WriterStream *)self); break;
	default: MemoryStream_destroy((MemoryStream *)self);
    }
    free(self);
}

