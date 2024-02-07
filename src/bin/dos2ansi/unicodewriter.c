#include "unicodewriter.h"

#include "stream.h"
#include "util.h"

#include <stdint.h>

typedef struct UnicodeWriter
{
    StreamWriter base;
    void (*outfunc)(Stream *stream, uint16_t c);
} UnicodeWriter;

static const char *magic = "";

static void toutf8(Stream *stream, uint16_t c)
{
    if (c < 0x80)
    {
	Stream_putc(stream, c);
	return;
    }
    unsigned char lb = c & 0xff;
    unsigned char hb = c >> 8;
    if (c < 0x800)
    {
	Stream_putc(stream, 0xc0U | (hb << 2) | (lb >> 6));
	Stream_putc(stream, 0x80U | (lb & 0x3fU));
    }
    else
    {
	Stream_putc(stream, 0xe0U | (hb >> 4));
	Stream_putc(stream, 0x80U | ((hb << 2) & 0x3fU) | (lb >> 6));
	Stream_putc(stream, 0x80U | (lb & 0x3fU));
    }
}

static void toutf16(Stream *stream, uint16_t c)
{
    Stream_putc(stream, c >> 8);
    Stream_putc(stream, c & 0xffU);
}

static void toutf16le(Stream *stream, uint16_t c)
{
    Stream_putc(stream, c & 0xffU);
    Stream_putc(stream, c >> 8);
}

static size_t write(StreamWriter *self, const void *ptr, size_t size)
{
    UnicodeWriter *writer = (UnicodeWriter *)self;
    if (size < 2) return 0;
    const uint16_t *unichr = ptr;
    writer->outfunc(self->stream, *unichr);
    return Stream_status(self->stream) == SS_OK ? 2 : 0;
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
    return Stream_createWriter((StreamWriter *)writer, magic);
}

