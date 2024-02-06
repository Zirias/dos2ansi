#include "ticolorwriter.h"

#include "stream.h"
#include "util.h"

#include <curses.h>
#include <stdint.h>
#include <stdlib.h>
#include <term.h>
#include <unistd.h>

static const int rgbcols[] = {
    16,   /* black */
    124,  /* red */
    34,   /* green */
    142,  /* dark yellow */
    19,   /* blue */
    127,  /* magenta */
    37,   /* cyan */
    248,  /* light gray */
    240,  /* dark gray */
    203,  /* light red */
    83,   /* light green */
    227,  /* yellow */
    63,   /* light blue */
    207,  /* light magenta */
    87,   /* light cyan */
    231   /* white */
};
#define RGBBROWN(i) ((i) == 142 ? 130 : (i))

typedef struct TiColorWriter
{
    StreamWriter base;
    const char *reset;
    const char *brightfg;
    const char *brightbg;
    const char *setaf;
    const char *setab;
    const int *rgbcols;
    ColorFlags flags;
    int fg;
    int bg;
} TiColorWriter;

static TiColorWriter *instance = 0;

static int putstream(int c)
{
    uint16_t unichr = c;
    if (Stream_write(instance->base.stream, &unichr, 2) != 2) return EOF;
    return c;
}

static size_t writeticol(StreamWriter *self, const void *ptr, size_t size)
{
    TiColorWriter *writer = (TiColorWriter *)self;
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
    int newfgcol = newfg;
    int newbgcol = newbg;
    if (defcols < 2 && newfg == writer->fg && newbg == writer->bg) return size;
    if (tputs(tigetstr(writer->reset), 1, putstream) == ERR) return 0;
    if (defcols && newbg == 0 && newfg == 7) goto done;
    if (writer->brightbg && (newbg & 8U))
    {
	if (*writer->brightbg && tputs(tigetstr(writer->brightbg),
		    1, putstream) == ERR) return 0;
	newbgcol &= 7U;
    }
    if (writer->brightfg)
    {
	if ((newfg & 8U))
	{
	    if (*writer->brightfg && tputs(tigetstr(writer->brightfg),
			1, putstream) == ERR) return 0;
	    newfgcol &= 7U;
	}
	newbgcol &= 7U;
    }
    if (writer->rgbcols)
    {
	newfgcol = writer->rgbcols[newfgcol];
	newbgcol = writer->rgbcols[newbgcol];
	if (!(writer->flags & CF_RGBNOBROWN))
	{
	    newfgcol = RGBBROWN(newfgcol);
	    newbgcol = RGBBROWN(newbgcol);
	}
    }
    if (tputs(tiparm(tigetstr(writer->setab), newbgcol),
		1, putstream) == ERR) return 0;
    if (tputs(tiparm(tigetstr(writer->setaf), newfgcol),
		1, putstream) == ERR) return 0;
done:
    writer->fg = defcols == 2 ? -1 : newfg;
    writer->bg = defcols == 2 ? -1 : newbg;
    return size;
}

static void destroyticol(StreamWriter *self)
{
    if (!self) return;
    Stream_destroy(self->stream);
    free(self);
    instance = 0;
}

Stream *TiColorWriter_create(Stream *out, ColorFlags flags)
{
    if (instance) return 0;
    TiColorWriter *writer = xmalloc(sizeof *writer);
    writer->base.write = writeticol;
    writer->base.flush = 0;
    writer->base.status = 0;
    writer->base.destroy = destroyticol;
    writer->base.stream = out;
    writer->brightfg = 0;
    writer->brightbg = 0;
    writer->rgbcols = 0;
    writer->fg = -1;
    writer->bg = -1;
    if (flags & CF_STRIP) goto error;
    int err;
    if (setupterm(0, Stream_file(out), &err) == ERR) goto error;
    int ncols = tigetnum("colors");
    if (ncols == ERR) ncols = tigetnum("Co");
    if (ncols == ERR || ncols < 8) goto error;
    if (tigetstr("sgr0")) writer->reset = "sgr0";
    else if (tigetstr("me")) writer->reset = "me";
    else goto error;
    if (tigetstr("setaf")) writer->setaf = "setaf";
    else if (tigetstr("AF")) writer->setaf = "AF";
    else goto error;
    if (tigetstr("setab")) writer->setab = "setab";
    else if (tigetstr("AB")) writer->setab = "AB";
    else goto error;
    if (ncols < 16)
    {
	if (tigetstr("bold")) writer->brightfg = "bold";
	else if (tigetstr("md")) writer->brightfg = "md";
	else writer->brightfg = "";
	if (flags & CF_LBG_REV)
	{
	    if (tigetstr("rev")) writer->brightbg = "rev";
	    else if (tigetstr("mr")) writer->brightbg = "mr";
	    else writer->brightbg = "";
	}
    }
    else
    {
	if (ncols >= 256)
	{
	    writer->rgbcols = rgbcols;
	}
	flags &= ~CF_LBG_REV;
    }
    if (flags & CF_LBG_BLINK)
    {
	if (tigetstr("blink")) writer->brightbg = "blink";
	else if (tigetstr("mb")) writer->brightbg = "mb";
	else writer->brightbg = "";
    }

    writer->flags = flags;
    goto done;

error:
    writer->flags = CF_STRIP;
done:
    instance = writer;
    return Stream_createWriter((StreamWriter *)writer, rgbcols);
}

