#include "unicodewriter.h"

#include "stream.h"
#include "util.h"

#include <stdint.h>

typedef struct UnicodeWriter
{
    StreamWriter base;
    int (*outfunc)(Stream *stream, uint16_t c);
} UnicodeWriter;

static int put(Stream *stream, unsigned char c)
{
    return Stream_write(stream, &c, 1);
}

static int toutf8(Stream *stream, uint16_t c)
{
    if (c < 0x80)
    {
	if (!put(stream, c)) return 0;
	return sizeof c;
    }
    unsigned char lb = c & 0xff;
    unsigned char hb = c >> 8;
    if (c < 0x800)
    {
	if (!put(stream, 0xc0U | (hb << 2) | (lb >> 6))) return 0;
	if (!put(stream, 0x80U | (lb & 0x3fU))) return 0;
    }
    else
    {
	if (!put(stream, 0xe0U | (hb >> 4))) return 0;
	if (!put(stream, 0x80U | ((hb << 2) & 0x3fU) | (lb >> 6))) return 0;
	if (!put(stream, 0x80U | (lb & 0x3fU))) return 0;
    }
    return sizeof c;
}

static int toutf16(Stream *stream, uint16_t c)
{
    if (!put(stream, c >> 8)) return 0;
    if (!put(stream, c & 0xffU)) return 0;
    return sizeof c;
}

static int toutf16le(Stream *stream, uint16_t c)
{
    if (!put(stream, c & 0xffU)) return 0;
    if (!put(stream, c >> 8)) return 0;
    return sizeof c;
}

static size_t write(StreamWriter *self, const void *ptr, size_t size)
{
    UnicodeWriter *writer = (UnicodeWriter *)self;
    const uint16_t *unichr = ptr;
    if (size != sizeof *unichr) return 0;
    return writer->outfunc(self->stream, *unichr);
}

Stream *UnicodeWriter_create(Stream *out, UnicodeFormat format)
{
    UnicodeWriter *writer = xmalloc(sizeof *writer);
    writer->base.write = write;
    writer->base.flush = 0;
    writer->base.status = 0;
    writer->base.destroy = 0;
    writer->base.stream = out;
    switch (format)
    {
	case UF_UTF8:
	    writer->outfunc = toutf8;
	    break;
	case UF_UTF16:
	    writer->outfunc = toutf16;
	    break;
	case UF_UTF16LE:
	    writer->outfunc = toutf16le;
	    break;
    }
    return Stream_createWriter((StreamWriter *)writer);
}

