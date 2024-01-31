#include "ansicolorwriter.h"

#include "stream.h"
#include "util.h"

#include <stdint.h>

static const char *rgbcols[] = {
    "16",   /* black */
    "124",  /* red */
    "34",   /* green */
    "142",  /* dark yellow */
    "19",   /* blue */
    "127",  /* magenta */
    "37",   /* cyan */
    "248",  /* light gray */
    "240",  /* dark gray */
    "203",  /* light red */
    "83",   /* light green */
    "227",  /* yellow */
    "63",   /* light blue */
    "207",  /* light magenta */
    "87",   /* light cyan */
    "231"   /* white */
};
static const char *rgbbrown = "130";
static const char *rgbinit = "8;5;";

typedef struct AnsiColorWriter
{
    StreamWriter base;
    ColorFlags flags;
    int bg;
    int fg;
} AnsiColorWriter;

static size_t put(Stream *stream, uint16_t c)
{
    return Stream_write(stream, &c, sizeof c);
}

static int putstr(Stream *stream, const char *str)
{
    while (*str)
    {
	if (!put(stream, *str++)) return 0;
    }
    return 1;
}

static int writergb(AnsiColorWriter *self,
	int newfg, int newbg, int defcols)
{
    char nextarg = '[';
    int oldbg = self->bg;
    if (self->flags & CF_LBG_BLINK)
    {
	if (((oldbg < 0 && (newbg & 8U)) || (newbg & 8U) != (oldbg & 8U)))
	{
	    if (!put(self->base.stream, nextarg)) return 0;
	    nextarg = ';';
	    if (newbg & 8U)
	    {
		if (!put(self->base.stream, '5')) return 0;
	    }
	    else
	    {
		if (!put(self->base.stream, '2')) return 0;
		if (!put(self->base.stream, '5')) return 0;
	    }
	}
	if (oldbg > 7) oldbg &= 7U;
	newbg &= 7U;
    }
    if ((oldbg < 0 && newbg != 0) || newbg != oldbg)
    {
	if (!put(self->base.stream, nextarg)) return 0;
	nextarg = ';';
	if (!put(self->base.stream, '4')) return 0;
	if (defcols && newbg == 0)
	{
	    if (!put(self->base.stream, '9')) return 0;
	}
	else 
	{
	    const char *colstr = rgbcols[newbg];
	    if (!(self->flags & CF_RGBNOBROWN) && newbg == 3)
	    {
		colstr = rgbbrown;
	    }
	    if (!putstr(self->base.stream, rgbinit)) return 0;
	    if (!putstr(self->base.stream, colstr)) return 0;
	}
    }
    if ((self->fg < 0 && newfg != 7) || newfg != self->fg)
    {
	if (!put(self->base.stream, nextarg)) return 0;
	if (!put(self->base.stream, '3')) return 0;
	if (defcols && newfg == 7)
	{
	    if (!put(self->base.stream, '9')) return 0;
	}
	else
	{
	    const char *colstr = rgbcols[newfg];
	    if (!(self->flags & CF_RGBNOBROWN) && newfg == 3)
	    {
		colstr = rgbbrown;
	    }
	    if (!putstr(self->base.stream, rgbinit)) return 0;
	    if (!putstr(self->base.stream, colstr)) return 0;
	}
    }
    return 1;
}

static int writebright(AnsiColorWriter *self,
	int newfg, int newbg, int defcols)
{
    char nextarg = '[';
    int oldbg = self->bg;
    if (self->flags & CF_LBG_BLINK)
    {
	if (((oldbg < 0 && (newbg & 8U)) || (newbg & 8U) != (oldbg & 8U)))
	{
	    if (!put(self->base.stream, nextarg)) return 0;
	    nextarg = ';';
	    if (newbg & 8U)
	    {
		if (!put(self->base.stream, '5')) return 0;
	    }
	    else
	    {
		if (!put(self->base.stream, '2')) return 0;
		if (!put(self->base.stream, '5')) return 0;
	    }
	}
	if (oldbg > 7) oldbg &= 7U;
	newbg &= 7U;
    }
    if ((oldbg < 0 && newbg != 0) || newbg != oldbg)
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
    if (self->flags & CF_LBG_REV) lightbg = '7';
    else if (self->flags & CF_LBG_BLINK) lightbg = '5';

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
    if (writer->flags & CF_STRIP) return size;
    int defcols = !!(writer->flags & CF_DEFAULT);
    if (c == 0xef00)
    {
	c = 0xee07;
	defcols = defcols ? 1 : 2;
    }
    int newfg = c & 0xfU;
    int newbg = (c >> 4) & 0xfU;
    if (defcols < 2 && newfg == writer->fg && newbg == writer->bg) return size;
    if (!put(self->stream, 0x1b)) return 0;
    if (defcols && newbg == 0 && newfg == 7)
    {
	if (!put(self->stream, '[')) return 0;
    }
    else if (writer->flags & CF_RGBCOLS)
    {
	if (!writergb(writer, newfg, newbg, defcols)) return 0;
    }
    else if (writer->flags & CF_BRIGHTCOLS)
    {
	if (!writebright(writer, newfg, newbg, defcols)) return 0;
    }
    else if (!writesimple(writer, newfg, newbg, defcols)) return 0;
    if (!put(self->stream, 'm')) return 0;
    writer->fg = defcols == 2 ? -1 : newfg;
    writer->bg = defcols == 2 ? -1 : newbg;
    return size;
}

Stream *AnsiColorWriter_create(Stream *out, ColorFlags flags)
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

