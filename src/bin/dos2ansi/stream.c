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

typedef struct FileStream
{
    struct Stream base;
    FILE *file;
} FileStream;

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
    self->base.size = 0;
    self->file = file;
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

size_t Stream_write(Stream *self, const void *ptr, size_t sz)
{
    if (self->size) return MemoryStream_write((MemoryStream *)self, ptr, sz);
    else return FileStream_write((FileStream *)self, ptr, sz);
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
    if (self->size) return MemoryStream_read((MemoryStream *)self, ptr, sz);
    else return FileStream_read((FileStream *)self, ptr, sz);
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

int Stream_status(const Stream *self)
{
    if (self->size) return MemoryStream_status((const MemoryStream *)self);
    else return FileStream_status((const FileStream *)self);
}

static void MemoryStream_destroy(MemoryStream *self)
{
    free(self->mem);
    free(self);
}

static void FileStream_destroy(FileStream *self)
{
    if (self->file != stdin && self->file != stdout && self->file != stderr)
    {
	fclose(self->file);
    }
    free(self);
}

void Stream_destroy(Stream *self)
{
    if (!self) return;
    if (self->size) MemoryStream_destroy((MemoryStream *)self);
    else FileStream_destroy((FileStream *)self);
}

