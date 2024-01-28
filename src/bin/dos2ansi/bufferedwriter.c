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

static int flush(StreamWriter *self)
{
    BufferedWriter *writer = (BufferedWriter *)self;
    if (writer->bufused)
    {
	size_t flushpos = 0;
	while (flushpos < writer->bufused)
	{
	    size_t written = Stream_write(self->stream,
		    writer->buf + flushpos, writer->bufused - flushpos);
	    if (!written) return EOF;
	    flushpos += written;
	}
	writer->bufused = 0;
    }
    return Stream_flush(self->stream);
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

    if (bufavail)
    {
	memcpy(writer->buf + writer->bufused, cptr, bufavail);
	cptr += bufavail;
	size -= bufavail;
	writer->bufused = writer->bufsize;
    }
    if (flush(self) < 0) return 0;
    if (size > writer->bufsize)
    {
	return bufavail + Stream_write(self->stream, cptr, size);
    }
    memcpy(writer->buf, cptr, size);
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
    return Stream_createWriter((StreamWriter *)writer);
}
