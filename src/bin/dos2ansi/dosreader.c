#include "dosreader.h"

#include "stream.h"
#include "util.h"

#include <string.h>

typedef struct DosReader
{
    StreamReader base;
    size_t bufsize;
    size_t bufused;
    size_t bufpos;
    long insz;
    long inpos;
    int ignoreeof;
    int eof;
    char buf[];
} DosReader;

static const char *magic = "";

static size_t read(StreamReader *self, void *ptr, size_t size)
{
    DosReader *reader = (DosReader *)self;
    char *cptr = ptr;
    size_t nread = 0;

    if (reader->eof) return 0;

    while (!reader->eof && nread < size)
    {
	if (reader->bufpos == reader->bufused) reader->bufused = 0;
	if (!reader->bufused)
	{
	    reader->bufpos = 0;
	    reader->bufused = Stream_read(self->stream,
		    reader->buf, reader->bufsize);
	    if (!reader->bufused) break;
	}
	size_t buflen = reader->bufused - reader->bufpos;
	if (buflen > size - nread) buflen = size - nread;
	if (!reader->ignoreeof)
	{
	    char *eofpos = memchr(reader->buf + reader->bufpos, 0x1a, buflen);
	    if (eofpos)
	    {
		buflen = eofpos - (reader->buf + reader->bufpos);
		reader->eof = 1;
	    }
	    if (reader->insz >= 0)
	    {
		reader->inpos += buflen;
		if (reader->inpos > reader->insz)
		{
		    buflen -= reader->inpos - reader->insz;
		    reader->eof = 1;
		    reader->inpos = reader->insz;
		}
	    }
	}
	if (!buflen) break;
	memcpy(cptr, reader->buf + reader->bufpos, buflen);
	cptr += buflen;
	nread += buflen;
	reader->bufpos += buflen;
	if (reader->bufused < reader->bufsize) break;
    }
    return nread;
}

static int status(const StreamReader *self)
{
    const DosReader *reader = (const DosReader *)self;
    if (reader->eof) return SS_EOF;
    return Stream_status(self->stream);
}

Stream *DosReader_create(Stream *in, size_t bufsize, long insz, int ignoreeof)
{
    DosReader *reader = xmalloc(sizeof *reader + bufsize);
    reader->base.read = read;
    reader->base.status = status;
    reader->base.destroy = 0;
    reader->base.stream = in;
    reader->bufsize = bufsize;
    reader->bufused = 0;
    reader->bufpos = 0;
    reader->insz = insz;
    reader->inpos = 0;
    reader->ignoreeof = ignoreeof;
    reader->eof = 0;
    return Stream_createReader((StreamReader *)reader, magic);
}

