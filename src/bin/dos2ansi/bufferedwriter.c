#include "bufferedwriter.h"

#include "stream.h"
#include "util.h"

#include <string.h>

typedef struct BufferedWriter
{
    StreamWriter base;
    size_t bufsize;
    size_t bufused;
    char buf[];
} BufferedWriter;

static const char *magic = "";

static int flush(StreamWriter *self, int sys)
{
    BufferedWriter *writer = (BufferedWriter *)self;
    if (writer->bufused)
    {
	if (!Stream_write(self->stream,
		    writer->buf, writer->bufused)) return -1;
	writer->bufused = 0;
    }
    return Stream_flush(self->stream, sys);
}

static size_t write(StreamWriter *self, const void *ptr, size_t size)
{
    BufferedWriter *writer = (BufferedWriter *)self;
    const char *cptr = ptr;

    size_t bufavail = writer->bufsize - writer->bufused;
    if (size <= bufavail)
    {
	memcpy(writer->buf + writer->bufused, cptr, size);
	writer->bufused += size;
	return size;
    }

    if (bufavail && size <= writer->bufsize)
    {
	memcpy(writer->buf + writer->bufused, cptr, bufavail);
	cptr += bufavail;
	size -= bufavail;
	writer->bufused = writer->bufsize;
    }
    else bufavail = 0;
    if (flush(self, 0) < 0) return 0;
    if (size > writer->bufsize)
    {
	return bufavail + Stream_write(self->stream, cptr, size);
    }
    memcpy(writer->buf, cptr, size);
    writer->bufused = size;
    return bufavail + size;
}

Stream *BufferedWriter_create(Stream *out, size_t bufsize)
{
    BufferedWriter *writer = xmalloc(sizeof *writer + bufsize);
    writer->base.write = write;
    writer->base.flush = flush;
    writer->base.status = 0;
    writer->base.destroy = 0;
    writer->base.stream = out;
    writer->bufsize = bufsize;
    writer->bufused = 0;
    return Stream_createWriter((StreamWriter *)writer, magic);
}

void BufferedWriter_discard(Stream *stream)
{
    StreamWriter *self = Stream_writer(stream, magic);
    if (!self) return;
    ((BufferedWriter *)self)->bufused = 0;
}
