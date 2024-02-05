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
    int ignoreeof;
    int doseof;
    char buf[];
} DosReader;

static const char *magic = "";

static size_t read(StreamReader *self, void *ptr, size_t size)
{
    DosReader *reader = (DosReader *)self;
    char *cptr = ptr;
    size_t nread = 0;

    if (reader->doseof > 0)
    {
	reader->doseof = -1;
	return 0;
    }

    reader->doseof = 0;
    while (!reader->doseof && nread < size)
    {
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
	    if (eofpos) buflen = eofpos - (reader->buf + reader->bufpos);
	}
	if (!buflen) break;
	if (cptr)
	{
	    memcpy(cptr, reader->buf + reader->bufpos, buflen);
	    cptr += buflen;
	}
	nread += buflen;
	reader->bufpos += buflen;
	if (!reader->ignoreeof && reader->bufpos < reader->bufused
		&& reader->buf[reader->bufpos] == 0x1a)
	{
	    ++reader->bufpos;
	    reader->doseof = 1;
	}
	if (reader->bufpos == reader->bufused) reader->bufused = 0;
    }
    return nread;
}

static int status(const StreamReader *self)
{
    const DosReader *reader = (const DosReader *)self;
    if (reader->doseof < 0) return SS_DOSEOF;
    return Stream_status(self->stream);
}

Stream *DosReader_create(Stream *in, size_t bufsize, int ignoreeof)
{
    DosReader *reader = xmalloc(sizeof *reader + bufsize);
    reader->base.read = read;
    reader->base.status = status;
    reader->base.destroy = 0;
    reader->base.stream = in;
    reader->bufsize = bufsize;
    reader->bufused = 0;
    reader->bufpos = 0;
    reader->ignoreeof = ignoreeof;
    reader->doseof = 0;
    return Stream_createReader((StreamReader *)reader, magic);
}

int DosReader_seekAfterEof(Stream *stream)
{
    StreamReader *self = Stream_reader(stream, magic);
    if (!self) return EOF;
    DosReader *reader = (DosReader *)self;
    while (read(self, 0, reader->bufsize)) ;
    if (!reader->doseof) return EOF;
    return 0;
}

