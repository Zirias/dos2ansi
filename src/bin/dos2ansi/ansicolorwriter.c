#include "ansicolorwriter.h"

#include "stream.h"
#include "util.h"

#include <stdint.h>

static const uint16_t rgbcols[][4] = {
    {'1','6', 0, 0},	/* black */
    {'1','2','4',0},	/* red */
    {'3','4', 0, 0},	/* green */
    {'1','4','2',0},	/* dark yellow */
    {'1','9', 0, 0},	/* blue */
    {'1','2','7',0},	/* magenta */
    {'3','7', 0, 0},	/* cyan */
    {'2','4','8',0},	/* light gray */
    {'2','4','0',0},	/* dark gray */
    {'2','0','3',0},	/* light red */
    {'8','3', 0, 0},	/* light green */
    {'2','2','7',0},	/* yellow */
    {'6','3', 0, 0},	/* light blue */
    {'2','0','7',0},	/* light magenta */
    {'8','7', 0, 0},	/* light cyan */
    {'2','3','1',0},	/* white */
};
static const uint16_t rgbbrown[] = {'1','3','0',0};
static const uint16_t rgbinit[] = {'8',';','5',';',0};

typedef struct AnsiColorWriter
{
    StreamWriter base;
    ColorFlags flags;
    int bg;
    int fg;
} AnsiColorWriter;

static size_t put(Stream *stream, uint16_t c)
{
    return Stream_write(stream, &c, 2);
}

static size_t putstr(Stream *stream, const uint16_t *str)
{
    size_t len = 0;
    while (str[len]) ++len;
    return Stream_write(stream, str, 2*len);
}

static void writergb(AnsiColorWriter *self,
	int newfg, int newbg, int defcols)
{
    char nextarg = '[';
    int oldbg = self->bg;
    if (self->flags & CF_LBG_BLINK)
    {
	if (((oldbg < 0 && (newbg & 8U)) || (newbg & 8U) != (oldbg & 8U)))
	{
	    put(self->base.stream, nextarg);
	    nextarg = ';';
	    if (newbg & 8U) put(self->base.stream, '5');
	    else
	    {
		put(self->base.stream, '2');
		put(self->base.stream, '5');
	    }
	}
	if (oldbg > 7) oldbg &= 7U;
	newbg &= 7U;
    }
    if ((oldbg < 0 && newbg != 0) || newbg != oldbg)
    {
	put(self->base.stream, nextarg);
	nextarg = ';';
	put(self->base.stream, '4');
	if (defcols && newbg == 0) put(self->base.stream, '9');
	else 
	{
	    const uint16_t *colstr = rgbcols[newbg];
	    if (!(self->flags & CF_RGBNOBROWN) && newbg == 3)
	    {
		colstr = rgbbrown;
	    }
	    putstr(self->base.stream, rgbinit);
	    putstr(self->base.stream, colstr);
	}
    }
    if ((self->fg < 0 && newfg != 7) || newfg != self->fg)
    {
	put(self->base.stream, nextarg);
	put(self->base.stream, '3');
	if (defcols && newfg == 7) put(self->base.stream, '9');
	else
	{
	    const uint16_t *colstr = rgbcols[newfg];
	    if (!(self->flags & CF_RGBNOBROWN) && newfg == 3)
	    {
		colstr = rgbbrown;
	    }
	    putstr(self->base.stream, rgbinit);
	    putstr(self->base.stream, colstr);
	}
    }
}

static void writebright(AnsiColorWriter *self,
	int newfg, int newbg, int defcols)
{
    char nextarg = '[';
    int oldbg = self->bg;
    if (self->flags & CF_LBG_BLINK)
    {
	if (((oldbg < 0 && (newbg & 8U)) || (newbg & 8U) != (oldbg & 8U)))
	{
	    put(self->base.stream, nextarg);
	    nextarg = ';';
	    if (newbg & 8U) put(self->base.stream, '5');
	    else
	    {
		put(self->base.stream, '2');
		put(self->base.stream, '5');
	    }
	}
	if (oldbg > 7) oldbg &= 7U;
	newbg &= 7U;
    }
    if ((oldbg < 0 && newbg != 0) || newbg != oldbg)
    {
	put(self->base.stream, nextarg);
	nextarg = ';';
	if (newbg & 8U)
	{
	    put(self->base.stream, '1');
	    put(self->base.stream, '0');
	}
	else put(self->base.stream, '4');
	if (defcols && newbg == 0) put(self->base.stream, '9');
	else put(self->base.stream, (newbg & 7U) + '0');
    }
    if ((self->fg < 0 && newfg != 7) || newfg != self->fg)
    {
	put(self->base.stream, nextarg);
	if (newfg & 8U) put(self->base.stream, '9');
	else put(self->base.stream, '3');
	if (defcols && newfg == 7) put(self->base.stream, '9');
	else put(self->base.stream, (newfg & 7U) + '0');
    }
}

static void writesimple(AnsiColorWriter *self,
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
	put(self->base.stream, nextarg);
	nextarg = ';';
	if (newbg & 8U) put(self->base.stream, lightbg);
	else
	{
	    put(self->base.stream, '2');
	    put(self->base.stream, lightbg);
	}
    }
    if ((self->fg < 0 && (newfg & 8U)) || (newfg & 8U) != (self->fg & 8U))
    {
	put(self->base.stream, nextarg);
	nextarg = ';';
	if (newfg & 8U) put(self->base.stream, '1');
	else
	{
	    put(self->base.stream, '2');
	    put(self->base.stream, '2');
	}
    }
    if ((self->bg < 0 && (newbg & 7U) != 0)
	    || (newbg & 7U) != (self->bg & 7U))
    {
	put(self->base.stream, nextarg);
	nextarg = ';';
	put(self->base.stream, '4');
	if (defcols && newbg == 0) put(self->base.stream, '9');
	else put(self->base.stream, (newbg & 7U) + '0');
    }
    if ((self->fg < 0 && (newfg & 7U) != 7)
	    || (newfg & 7U) != (self->fg & 7U))
    {
	put(self->base.stream, nextarg);
	put(self->base.stream, '3');
	if (defcols && newfg == 7) put(self->base.stream, '9');
	else put(self->base.stream, (newfg & 7U) + '0');
    }
}

static size_t write(StreamWriter *self, const void *ptr, size_t size)
{
    AnsiColorWriter *writer = (AnsiColorWriter *)self;
    if (size < 2) return 0;
    const uint16_t *unichr = ptr;
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
    put(self->stream, 0x1b);
    if (defcols && newbg == 0 && newfg == 7) put(self->stream, '[');
    else if (writer->flags & CF_RGBCOLS)
	writergb(writer, newfg, newbg, defcols);
    else if (writer->flags & CF_BRIGHTCOLS)
	writebright(writer, newfg, newbg, defcols);
    else writesimple(writer, newfg, newbg, defcols);
    if (!put(self->stream, 'm')) return 0;
    writer->fg = defcols == 2 ? -1 : newfg;
    writer->bg = defcols == 2 ? -1 : newbg;
    return 2;
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
    return Stream_createWriter((StreamWriter *)writer, rgbbrown);
}

