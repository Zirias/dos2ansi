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

static size_t put(Stream *stream, uint16_t c)
{
    return Stream_write(stream, &c, sizeof c);
}

static int writebright(AnsiColorWriter *self,
	int newfg, int newbg, int defcols)
{
    char nextarg = '[';
    if ((self->bg < 0 && newbg != 0) || newbg != self->bg)
    {
	if (!put(self->base.stream, nextarg)) return 0;
	nextarg = ';';
	if (newbg & 8U)
	{
	    if (!put(self->base.stream, '1')) return 0;
	    if (!put(self->base.stream, '0')) return 0;
	}
	else if (!put(self->base.stream, '4')) return 0;
	if (defcols && newbg == 0)
	{
	    if (!put(self->base.stream, '9')) return 0;
	}
	else if (!put(self->base.stream, (newbg & 7U) + '0')) return 0;
    }
    if ((self->fg < 0 && newfg != 7) || newfg != self->fg)
    {
	if (!put(self->base.stream, nextarg)) return 0;
	if (newfg & 8U)
	{
	    if (!put(self->base.stream, '9')) return 0;
	}
	else if (!put(self->base.stream, '3')) return 0;
	if (defcols && newfg == 7)
	{
	    if (!put(self->base.stream, '9')) return 0;
	}
	else if (!put(self->base.stream, (newfg & 7U) + '0')) return 0;
    }
    return 1;
}

static int writesimple(AnsiColorWriter *self,
	int newfg, int newbg, int defcols)
{
    char nextarg = '[';
    char lightbg = 0;
    if (self->flags & ACF_LBG_REV) lightbg = '7';
    else if (self->flags & ACF_LBG_BLINK) lightbg = '5';

    if (lightbg &&
	    ((self->bg < 0 && (newbg & 8U))
	     || (newbg & 8U) != (self->bg & 8U)))
    {
	if (!put(self->base.stream, nextarg)) return 0;
	nextarg = ';';
	if (newbg & 8U)
	{
	    if (!put(self->base.stream, lightbg)) return 0;
	}
	else
	{
	    if (!put(self->base.stream, '2')) return 0;
	    if (!put(self->base.stream, lightbg)) return 0;
	}
    }
    if ((self->fg < 0 && (newfg & 8U)) || (newfg & 8U) != (self->fg & 8U))
    {
	if (!put(self->base.stream, nextarg)) return 0;
	nextarg = ';';
	if (newfg & 8U)
	{
	    if (!put(self->base.stream, '1')) return 0;
	}
	else
	{
	    if (!put(self->base.stream, '2')) return 0;
	    if (!put(self->base.stream, '2')) return 0;
	}
    }
    if ((self->bg < 0 && (newbg & 7U) != 0)
	    || (newbg & 7U) != (self->bg & 7U))
    {
	if (!put(self->base.stream, nextarg)) return 0;
	nextarg = ';';
	if (!put(self->base.stream, '4')) return 0;
	if (defcols && newbg == 0)
	{
	    if (!put(self->base.stream, '9')) return 0;
	}
	else if (!put(self->base.stream, (newbg & 7U) + '0')) return 0;
    }
    if ((self->fg < 0 && (newfg & 7U) != 7)
	    || (newfg & 7U) != (self->fg & 7U))
    {
	if (!put(self->base.stream, nextarg)) return 0;
	if (!put(self->base.stream, '3')) return 0;
	if (defcols && newfg == 7)
	{
	    if (!put(self->base.stream, '9')) return 0;
	}
	else if (!put(self->base.stream, (newfg & 7U) + '0')) return 0;
    }
    return 1;
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
    if (c == 0xef00)
    {
	c = 0xee07;
	defcols = 1;
    }
    int newfg = c & 0xfU;
    int newbg = (c >> 4) & 0xfU;
    if (defcols && newbg == 0 && newfg == 7)
    {
	if (!put(self->stream, '[')) return 0;
    }
    else if (writer->flags & ACF_BRIGHTCOLS)
    {
	if (!writebright(writer, newfg, newbg, defcols)) return 0;
    }
    else
    {
	if (!writesimple(writer, newfg, newbg, defcols)) return 0;
    }
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

