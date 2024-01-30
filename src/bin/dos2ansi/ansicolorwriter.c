#include "ansicolorwriter.h"

#include "stream.h"
#include "util.h"

#include <stdint.h>

typedef struct AnsiColorWriter
{
    StreamWriter base;
    AnsiColorFlags flags;
    int bg;
    int fg;
} AnsiColorWriter;

static int put(Stream *stream, uint16_t c)
{
    return Stream_write(stream, &c, sizeof c);
}

static size_t write(StreamWriter *self, const void *ptr, size_t size)
{
    AnsiColorWriter *writer = (AnsiColorWriter *)self;
    const uint16_t *unichr = ptr;
    if (size != sizeof *unichr) return 0;
    uint16_t c = *unichr;
    if (c < 0xee00 || c > 0xef00)
    {
	return Stream_write(self->stream, ptr, size);
    }
    if (writer->flags & ACF_STRIP) return size;
    if (!put(self->stream, 0x1b)) return 0;
    int defcols = !!(writer->flags & ACF_DEFAULT);
    char lightbg = 0;
    if (writer->flags & ACF_LBG_REV) lightbg = '7';
    else if (writer->flags & ACF_LBG_BLINK) lightbg = '5';
    if (c == 0xef00)
    {
	c = 0xee07;
	defcols = 1;
    }
    int newfg = c & 0xfU;
    int newbg = (c >> 4) & 0xfU;
    char nextarg = '[';
    if (defcols && newbg == 0 && newfg == 7)
    {
	if (!put(self->stream, nextarg)) return 0;
	goto done;
    }
    if (lightbg &&
	    ((writer->bg < 0 && (newbg & 8U))
	     || (newbg & 8U) != (writer->bg & 8U)))
    {
	if (!put(self->stream, nextarg)) return 0;
	nextarg = ';';
	if (newbg & 8U)
	{
	    if (!put(self->stream, lightbg)) return 0;
	}
	else
	{
	    if (!put(self->stream, '2')) return 0;
	    if (!put(self->stream, lightbg)) return 0;
	}
    }
    if ((writer->fg < 0 && (newfg & 8U)) || (newfg & 8U) != (writer->fg & 8U))
    {
	if (!put(self->stream, nextarg)) return 0;
	nextarg = ';';
	if (newfg & 8U)
	{
	    if (!put(self->stream, '1')) return 0;
	}
	else
	{
	    if (!put(self->stream, '2')) return 0;
	    if (!put(self->stream, '2')) return 0;
	}
    }
    if ((writer->bg < 0 && (newbg & 7U) != 0)
	    || (newbg & 7U) != (writer->bg & 7U))
    {
	if (!put(self->stream, nextarg)) return 0;
	nextarg = ';';
	if (!put(self->stream, '4')) return 0;
	if (defcols && newbg == 0)
	{
	    if (!put(self->stream, '9')) return 0;
	}
	else if (!put(self->stream, (newbg & 7U) + '0')) return 0;
    }
    if ((writer->fg < 0 && (newfg & 7U) != 7)
	    || (newfg & 7U) != (writer->fg & 7U))
    {
	if (!put(self->stream, nextarg)) return 0;
	nextarg = ';';
	if (!put(self->stream, '3')) return 0;
	if (defcols && newfg == 0x07)
	{
	    if (!put(self->stream, '9')) return 0;
	}
	else if (!put(self->stream, (newfg & 7U) + '0')) return 0;
    }
done:
    if (!put(self->stream, 'm')) return 0;
    writer->fg = newfg;
    writer->bg = newbg;
    return size;
}

Stream *AnsiColorWriter_create(Stream *out, AnsiColorFlags flags)
{
    AnsiColorWriter *writer = xmalloc(sizeof *writer);
    writer->base.write = write;
    writer->base.flush = 0;
    writer->base.status = 0;
    writer->base.destroy = 0;
    writer->base.stream = out;
    writer->flags = flags;
    writer->bg = -1;
    writer->fg = -1;
    return Stream_createWriter((StreamWriter *)writer);
}

