#include "ansitermwriter.h"

#include "codepage.h"
#include "stream.h"
#include "vgacanvas.h"

#include <stdint.h>
#include <string.h>

static int usecolors = 1;
static int usedefcols = 0;
static int usebom = 1;
static int crlf = 0;
static int markltr = 0;

static int out(Stream *stream, uint16_t c)
{
    return -(Stream_write(stream, &c, sizeof c) != sizeof c);
}

static int writeansidc(Stream *stream, int newbg, int bg, int newfg, int fg,
	int defcols)
{
    char nextarg = '[';
    if (out(stream, 0x1b) != 0) return -1;
    if (defcols && newbg == 0x00 && newfg == 0x07)
    {
	if (out(stream, nextarg) != 0) return -1;
	goto done;
    }
    if (bg < 0 || (newbg & 0x08U) != (bg & 0x08U))
    {
	if (out(stream, nextarg) != 0) return -1;
	nextarg = ';';
	if (newbg & 0x08U)
	{
	    if (out(stream, '5') != 0) return -1;
	}
	else
	{
	    if (out(stream, '2') != 0) return -1;
	    if (out(stream, '5') != 0) return -1;
	}
    }
    if (fg < 0 || (newfg & 0x08U) != (fg & 0x08U))
    {
	if (out(stream, nextarg) != 0) return -1;
	nextarg = ';';
	if (newfg & 0x08U)
	{
	    if (out(stream, '1') != 0) return -1;
	}
	else
	{
	    if (out(stream, '2') != 0) return -1;
	    if (out(stream, '2') != 0) return -1;
	}
    }
    if (bg < 0 || (newbg & 0x07U) != (bg & 0x07U))
    {
	if (out(stream, nextarg) != 0) return -1;
	nextarg = ';';
	if (out(stream, '4') != 0) return -1;
	if (defcols && newbg == 0x00)
	{
	    if (out(stream, '9') != 0) return -1;
	}
	else if (out(stream, (newbg & 0x07U) + '0') != 0) return -1;
    }
    if (fg < 0 || (newfg & 0x07U) != (fg & 0x07U))
    {
	if (out(stream, nextarg) != 0) return -1;
	nextarg = ';';
	if (out(stream, '3') != 0) return -1;
	if (defcols && newfg == 0x07)
	{
	    if (out(stream, '9') != 0) return -1;
	}
	else if (out(stream, (newfg & 0x07U) + '0') != 0) return -1;
    }
done:
    return out(stream, 'm');
}

static int writeansi(Stream *stream, int newbg, int bg, int newfg, int fg)
{
    return writeansidc(stream, newbg, bg, newfg, fg, usedefcols);
}

void AnsiTermWriter_usecolors(int arg)
{
    usecolors = !!arg;
}

void AnsiTermWriter_usedefcols(int arg)
{
    usedefcols = !!arg;
}

void AnsiTermWriter_usebom(int arg)
{
    usebom = !!arg;
}

void AnsiTermWriter_crlf(int arg)
{
    crlf = !!arg;
}

void AnsiTermWriter_markltr(int arg)
{
    markltr = !!arg;
}

int AnsiTermWriter_write(Stream *stream, const Codepage *cp,
	const VgaCanvas *canvas)
{
    int bg = -1;
    int fg = -1;
    if (usedefcols)
    {
	bg = 0x00;
	fg = 0x07;
    }

    size_t height = VgaCanvas_height(canvas);
    if (!height) return 0;

    if (usebom && out(stream, 0xfeffU) != 0) return -1;
    if (markltr && out(stream, 0x202dU) != 0) return -1;

    for (size_t i = 0; i < height; ++i)
    {
	const VgaLine *line = VgaCanvas_line(canvas, i);
	int len = VgaLine_len(line);
	for (int j = 0; j < len; ++j)
	{
	    if (usecolors && VgaCanvas_hascolor(canvas))
	    {
		int newbg = VgaLine_bg(line, j);
		int newfg = VgaLine_fg(line, j);
		if (newbg != bg || newfg != fg)
		{
		    if (writeansi(stream, newbg, bg, newfg, fg) != 0) return -1;
		    bg = newbg;
		    fg = newfg;
		}
	    }
	    if (out(stream, Codepage_map(cp,
			    VgaLine_chr(line, j))) != 0) return -1;
	}
	if (usecolors && VgaCanvas_hascolor(canvas))
	{
	    if (i < height-1)
	    {
		if (writeansi(stream, bg & 0x08U, bg, fg, fg) != 0) return -1;
		bg &= 0x08U;
	    }
	    else if (writeansidc(stream, 0, bg, 0x07U, fg, 1) != 0) return -1;
	}
	if (crlf && out(stream, '\r') != 0) return -1;
	if (out(stream, '\n') != 0) return -1;
    }
    if (markltr && out(stream, 0x202cU) != 0) return -1;
    return Stream_flush(stream);
}

