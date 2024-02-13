#include "ticolorwriter.h"

#include "stream.h"
#include "util.h"

#include <curses.h>
#include <stdint.h>
#include <stdlib.h>
#include <term.h>
#include <unistd.h>

#ifdef USE_POSIX
#define STREAMFILENO(s) Stream_file(s)
#else
#define STREAMFILENO(s) fileno((FILE *)Stream_file(s))
#endif

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

#define tpstr(s) tputs((s), 1, putstream)

typedef struct TiColorWriter
{
    StreamWriter base;
    char *brightfg;
    char *brightbg;
    const int *rgbcols;
    ColorFlags flags;
    int fg;
    int bg;
} TiColorWriter;

static TiColorWriter *instance = 0;

static int putstream(int c)
{
    uint16_t unichr = c;
    if (Stream_write(instance->base.stream, &unichr, 2) != 2) return -1;
    return c;
}

static size_t writeticol(StreamWriter *self, const void *ptr, size_t size)
{
    TiColorWriter *writer = (TiColorWriter *)self;
    const uint16_t *unichr = ptr;
    if (size < 2) return 0;
    uint16_t c = *unichr;
    if (c < 0xee00 || c > 0xef00)
    {
	return Stream_write(self->stream, ptr, 2);
    }
    if (writer->flags & CF_STRIP) return 2;
    int defcols = !!(writer->flags & CF_DEFAULT);
    if (c == 0xef00)
    {
	if (tpstr(exit_attribute_mode) == ERR) return 0;
	writer->fg = defcols ? 7 : -1;
	writer->bg = defcols ? 0 : -1;
	return 2;
    }
    int newfg = c & 0xfU;
    int newbg = (c >> 4) & 0xfU;
    if (newfg == writer->fg && newbg == writer->bg) return 2;
    if (defcols && newfg == 7 && newbg == 0)
    {
	if (tpstr(exit_attribute_mode) == ERR) return 0;
	writer->fg = 7;
	writer->bg = 0;
	return 2;
    }
    int clearbgattr = writer->brightbg && ((writer->bg > 7) && !(newbg & 8U));
    int clearfgattr = writer->brightfg && ((writer->fg > 7) && !(newfg & 8U));
    if (defcols)
    {
	if (!writer->brightbg) clearbgattr = (writer->bg != 0 && newbg == 0);
	else clearbgattr = clearbgattr
	    || ((writer->bg & 7U) != 0 && (newbg & 7U) == 0);
	if (!writer->brightfg) clearfgattr = (writer->fg != 7 && newfg == 7);
	else clearfgattr = clearfgattr
	    || ((writer->fg & 7U) != 7 && (newfg & 7U) == 7);
    }
    if (clearbgattr || clearfgattr)
    {
	if (tpstr(exit_attribute_mode) == ERR) return 0;
	if (defcols)
	{
	    writer->fg = 7;
	    writer->bg = 0;
	    if (newfg == 7 && newbg == 0) return 2;
	}
	else
	{
	    writer->fg = -1;
	    writer->bg = -1;
	}
    }
    int newbgcol = newbg;
    int newfgcol = newfg;
    int oldfgcol = writer->fg;
    int oldbgcol = writer->bg;
    if (writer->brightbg)
    {
	if ((oldbgcol < 0 || !(oldbgcol & 8U)) && (newbgcol & 8U))
	{
	    if (*writer->brightbg && tpstr(writer->brightbg) == ERR) return 0;
	}
	if (oldbgcol > 7) oldbgcol &= 7U;
	newbgcol &= 7U;
    }
    if (writer->brightfg)
    {
	if ((oldfgcol < 0 || !(oldfgcol & 8U)) && (newfgcol & 8U))
	{
	    if (*writer->brightfg && tpstr(writer->brightfg) == ERR) return 0;
	    newfgcol &= 7U;
	}
	if (newfgcol > 7) newfgcol &= 7U;
	newfgcol &= 7U;
    }
    if (oldbgcol < 0 || oldbgcol != newbgcol)
    {
	if (writer->rgbcols)
	{
	    newbgcol = writer->rgbcols[newbgcol];
	    if (!(writer->flags & CF_RGBNOBROWN))
		newbgcol = RGBBROWN(newbgcol);
	}
	if (tpstr(tiparm(set_a_background, newbgcol)) == ERR) return 0;
    }
    if (oldfgcol < 0 || oldfgcol != newfgcol)
    {
	if (writer->rgbcols)
	{
	    newfgcol = writer->rgbcols[newfgcol];
	    if (!(writer->flags & CF_RGBNOBROWN))
		newfgcol = RGBBROWN(newfgcol);
	}
	if (tpstr(tiparm(set_a_foreground, newfgcol)) == ERR) return 0;
    }
    writer->fg = newfg;
    writer->bg = newbg;
    return 2;
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
    int defcols = !!(flags & CF_DEFAULT);
    TiColorWriter *writer = xmalloc(sizeof *writer);
    writer->base.write = writeticol;
    writer->base.flush = 0;
    writer->base.status = 0;
    writer->base.destroy = destroyticol;
    writer->base.stream = out;
    writer->brightfg = 0;
    writer->brightbg = 0;
    writer->rgbcols = 0;
    writer->fg = defcols ? 7 : -1;
    writer->bg = defcols ? 0 : -1;

    if (flags & CF_STRIP) goto error;
    int err;
    if (setupterm(0, STREAMFILENO(out), &err) == ERR) goto error;
    if (max_colors < 8) goto error;
    if (!exit_attribute_mode) goto error;
    if (!set_a_foreground) goto error;
    if (!set_a_background) goto error;

    if (max_colors < 16)
    {
	writer->brightfg = enter_bold_mode ? enter_bold_mode : "";
	if (flags & CF_LBG_REV)
	{
	    writer->brightbg = enter_reverse_mode ? enter_reverse_mode : "";
	}
    }
    else
    {
	if (max_colors >= 256)
	{
	    writer->rgbcols = rgbcols;
	}
	flags &= ~CF_LBG_REV;
    }
    if (flags & CF_LBG_BLINK)
    {
	writer->brightbg = enter_blink_mode ? enter_blink_mode : "";
    }

    writer->flags = flags;
    goto done;

error:
    writer->flags = CF_STRIP;
done:
    instance = writer;
    return Stream_createWriter((StreamWriter *)writer, rgbcols);
}

